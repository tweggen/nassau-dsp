#!/usr/bin/env bash
#
# ci/gates.sh — nassau-dsp's gate, in one command, locally and in CI.
#
#     ./ci/gates.sh
#
# 1. tools/check_headers.py: R2, standard headers only under Source/DSP/.
# 2. Configure, build and run the suite in its own tree, build-ci/.
#
# The logic lives here, not in the workflow file, so a move off GitHub
# rewrites the YAML that calls this and not the gate.
set -euo pipefail

REPO_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD_DIR="$REPO_DIR/build-ci"
BUILD_TYPE="${BUILD_TYPE:-Release}"
PYTHON="${PYTHON:-$(command -v python3 || command -v python)}"

say() { printf '\n=== %s ===\n' "$*"; }

say "Header check (R2)"
"$PYTHON" "$REPO_DIR/tools/check_headers.py"

say "Configure"
cmake -S "$REPO_DIR" -B "$BUILD_DIR" -DCMAKE_BUILD_TYPE="$BUILD_TYPE"

say "Build"
# --config/-C as well as CMAKE_BUILD_TYPE: a multi-config generator (Visual
# Studio) ignores the latter and would otherwise build Debug and find no tests.
cmake --build "$BUILD_DIR" --config "$BUILD_TYPE" -j

say "Test"
ctest --test-dir "$BUILD_DIR" -C "$BUILD_TYPE" --output-on-failure

say "Pass"
