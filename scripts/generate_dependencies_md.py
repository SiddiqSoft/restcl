#!/usr/bin/env python3
"""
generate_dependencies_md.py

Parses CMakeLists.txt files in the repository to extract project dependencies
and generates dependencies.md with a Mermaid dependency graph and breakdown table.
"""

import os
import re
import sys
import argparse
from pathlib import Path

# Friendly name overrides for known repositories/packages
DISPLAY_NAMES = {
    "compile-time-regular-expressions": "ctre",
    "json": "nlohmann_json",
    "CURL": "libcurl",
}


def parse_cmake_file(filepath):
    """
    Parses a CMakeLists.txt file to extract find_package and CPMAddPackage dependencies.
    """
    filepath = Path(filepath)
    if not filepath.exists():
        return []

    with open(filepath, "r", encoding="utf-8") as f:
        content = f.read()

    dependencies = []
    lines = content.splitlines()
    current_scope = "Core"
    is_test_file = "tests" in str(filepath)

    for line in lines:
        stripped = line.strip()

        # Update scope tracking based on CMake conditional blocks
        if "MATCHES [Mm][Ss][Vv][Cc]" in stripped:
            current_scope = "Windows"
        elif (
            "MATCHES [Cc][Ll][Aa][Nn][Gg]" in stripped
            or "MATCHES [Gg][Nn][Uu]" in stripped
            or "MATCHES [Aa][Pp][Pp][Ll][Ee]" in stripped
        ):
            current_scope = "Linux/macOS"
        elif stripped.startswith("endif()") or stripped.startswith("elseif("):
            if "MATCHES" not in stripped:
                current_scope = "Core"

        scope = "Test" if is_test_file else current_scope

        # Match CPMAddPackage("gh:owner/repo#version") or CPMAddPackage('...')
        cpm_match = re.search(
            r'CPMAddPackage\s*\(\s*["\'](?:gh:)?([^"\'#]+)#([^"\'\)]+)["\']\s*\)',
            stripped,
            re.IGNORECASE,
        )
        if cpm_match:
            repo_spec = cpm_match.group(1).strip()
            raw_version = cpm_match.group(2).strip()
            raw_name = repo_spec.split("/")[-1]
            name = DISPLAY_NAMES.get(raw_name, raw_name)
            dependencies.append(
                {
                    "name": name,
                    "repo": repo_spec,
                    "version": raw_version,
                    "type": "CPM",
                    "scope": scope,
                }
            )
            continue

        # Match find_package(PackageName Version ...)
        fp_match = re.search(
            r"find_package\s*\(\s*([A-Za-z0-9_]+)\s+([0-9\.]+)?", stripped
        )
        if fp_match:
            pkg_name = fp_match.group(1)
            pkg_ver = fp_match.group(2) or "REQUIRED"
            name = DISPLAY_NAMES.get(pkg_name, pkg_name)
            # Avoid duplicate entries for multi-branch find_package (e.g. Linux vs macOS)
            if not any(d["name"] == name for d in dependencies):
                dependencies.append(
                    {
                        "name": name,
                        "repo": f"System / {pkg_name}",
                        "version": f">= {pkg_ver}"
                        if pkg_ver != "REQUIRED"
                        else "System",
                        "type": "find_package",
                        "scope": scope if scope != "Core" else "Linux/macOS",
                    }
                )

    return dependencies


def generate_markdown(dependencies, project_name="restcl"):
    """
    Generates markdown content containing the Mermaid diagram and dependency breakdown table.
    """
    platform_deps = [
        d for d in dependencies if d["scope"] in ("Windows", "Linux/macOS")
    ]
    core_deps = [d for d in dependencies if d["scope"] == "Core"]
    test_deps = [d for d in dependencies if d["scope"] == "Test"]

    lines = []
    lines.append("# Project Dependencies")
    lines.append("")
    lines.append(
        f"This document is automatically generated from `CMakeLists.txt` files for `{project_name}`."
    )
    lines.append("")
    lines.append("## Dependency Diagram")
    lines.append("")
    lines.append("```mermaid")
    lines.append("graph TD")
    lines.append(f'    {project_name}["{project_name}::{project_name}"]')
    lines.append("")

    if platform_deps:
        lines.append(
            '    subgraph Platform["Platform-Specific HTTP Backend"]'
        )
        for d in platform_deps:
            node_id = re.sub(r"[^A-Za-z0-9]", "", d["name"]).upper()
            scope_desc = (
                "Windows / MSVC"
                if d["scope"] == "Windows"
                else "Linux / macOS"
            )
            lines.append(
                f'        {node_id}["{d["name"]} {d["version"]} ({scope_desc})"]'
            )
        lines.append("    end")
        lines.append("")

    if core_deps:
        lines.append('    subgraph Core["Core Dependencies (via CPM)"]')
        for d in core_deps:
            node_id = re.sub(r"[^A-Za-z0-9]", "", d["name"]).upper()
            lines.append(f'        {node_id}["{d["name"]} {d["version"]}"]')
        lines.append("    end")
        lines.append("")

    if test_deps:
        lines.append('    subgraph Test["Test Dependencies (Optional)"]')
        for d in test_deps:
            node_id = re.sub(r"[^A-Za-z0-9]", "", d["name"]).upper()
            lines.append(f'        {node_id}["{d["name"]} {d["version"]}"]')
        lines.append("    end")
        lines.append("")

    # Connect diagram edges
    for d in platform_deps + core_deps:
        node_id = re.sub(r"[^A-Za-z0-9]", "", d["name"]).upper()
        lines.append(f"    {project_name} --> {node_id}")

    for d in test_deps:
        node_id = re.sub(r"[^A-Za-z0-9]", "", d["name"]).upper()
        lines.append(
            f'    {project_name} -. "BUILD_TESTS=ON" .-> {node_id}'
        )

    lines.append("```")
    lines.append("")
    lines.append("## Dependency Breakdown")
    lines.append("")
    lines.append(
        "| Dependency | Repository / Target | Version | Type | Scope / Platform |"
    )
    lines.append(
        "| :--- | :--- | :--- | :--- | :--- |"
    )

    all_deps = platform_deps + core_deps + test_deps
    for d in all_deps:
        if d["repo"].startswith("System /"):
            repo_str = f'`{d["repo"]}`'
        else:
            repo_str = f'[`{d["repo"]}`](https://github.com/{d["repo"]})'

        scope_str = {
            "Windows": "Windows (MSVC)",
            "Linux/macOS": "Linux / macOS (GCC, Clang, AppleClang)",
            "Core": "All Platforms (`INTERFACE`)",
            "Test": "Test Target Only (`BUILD_TESTS=ON`)",
        }.get(d["scope"], d["scope"])

        lines.append(
            f'| **{d["name"]}** | {repo_str} | {d["version"]} | `{d["type"]}` | {scope_str} |'
        )

    lines.append("")
    return "\n".join(lines)


def main():
    parser = argparse.ArgumentParser(
        description="Extract CMake dependencies into dependencies.md with Mermaid diagram."
    )
    parser.add_argument(
        "--root",
        type=str,
        default=".",
        help="Root directory of the project (default: current directory)",
    )
    parser.add_argument(
        "--output",
        type=str,
        default="dependencies.md",
        help="Path to output markdown file (default: dependencies.md)",
    )
    parser.add_argument(
        "--also-output",
        type=str,
        default="docs/integration/dependencies.md",
        help="Additional file path to write to (default: docs/integration/dependencies.md)",
    )

    args = parser.parse_args()

    root_path = Path(args.root).resolve()
    cmake_files = [
        root_path / "CMakeLists.txt",
        root_path / "tests" / "CMakeLists.txt",
    ]

    all_deps = []
    for cm_file in cmake_files:
        if cm_file.exists():
            deps = parse_cmake_file(cm_file)
            all_deps.extend(deps)

    markdown_content = generate_markdown(all_deps, project_name="restcl")

    outputs = [Path(args.output)]
    if args.also_output:
        outputs.append(Path(args.also_output))

    for out_path in outputs:
        full_out_path = root_path / out_path if not out_path.is_absolute() else out_path
        full_out_path.parent.mkdir(parents=True, exist_ok=True)
        with open(full_out_path, "w", encoding="utf-8") as f:
            f.write(markdown_content)
        print(f"[generate_dependencies_md] Wrote dependency documentation to: {full_out_path}")


if __name__ == "__main__":
    main()
