import os
import sys
import subprocess
from pathlib import Path

def on_config(config, **kwargs):
    """
    MkDocs hook that dynamically injects the build version into the site configuration (config['extra']['version']).
    It resolves the version from CI environment variables (GitVersion / Azure Pipelines / GitHub Actions),
    or falls back to git describe / local default.
    """
    # 1. Resolve version from CI/GitVersion environment variables
    version = (
        os.getenv("GITVERSION_SEMVER")
        or os.getenv("GITVERSION_MAJORMINORPATCH")
        or os.getenv("BUILD_VERSION")
        or os.getenv("CI_BUILDID")
    )
    
    # 2. If not running in CI or env vars are missing, try running git describe
    if not version:
        try:
            version = subprocess.check_output(
                ["git", "describe", "--tags", "--always", "--dirty"],
                stderr=subprocess.DEVNULL,
                text=True
            ).strip()
        except Exception:
            version = "0.0.0-dev"

    # 3. Automatically regenerate dependencies documentation from CMakeLists.txt
    try:
        root_dir = Path(__file__).resolve().parent.parent
        script_path = root_dir / "scripts" / "generate_dependencies_md.py"
        if script_path.exists():
            subprocess.run([sys.executable, str(script_path), "--root", str(root_dir)], check=True)
    except Exception as e:
        print(f"[docs/hooks.py] Warning: Failed to generate dependencies.md: {e}")

    # 4. Inject version into MkDocs extra configuration
    if "extra" not in config or config["extra"] is None:
        config["extra"] = {}
    config["extra"]["version"] = version

    print(f"[docs/hooks.py] Resolved build version: {version}")
    return config

