#!/usr/bin/env bash
set -e

echo "Building MkDocs Material site..."
mkdocs build --config-file ../mkdocs.yml
