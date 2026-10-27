#!/usr/bin/env bash
#
# ci/build.sh — build nassau-dsp's tests for development, into build/.
#
#     ./ci/build.sh            # configure if needed, build
#     ./ci/build.sh --clean    # drop build/ first
#     ./ci/build.sh --test     # ...and run ctest afterwards
#     ./ci/build.sh --debug    # CMAKE_BUILD_TYPE=Debug (default: Release)
#
# The library itself is header-only; what builds here is its test suite. The
# gate (ci/gates.sh) uses its own tree, build-ci/, so running one does not
# invalidate the other. No SDK, no Qt, nothing to locate.
set -euo pipefail

REPO_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD_DIR="$REPO_DIR/build"
BUILD_TYPE="${BUILD_TYPE:-Release}"
CLEAN=""
RUN_TESTS=""

while [ $# -gt 0 ]; do
    case "$1" in
        --clean) CLEAN="yes"; shift ;;
        --test)  RUN_TESTS="yes"; shift ;;
        --debug) BUILD_TYPE="Debug"; shift ;;
        -h|--help) sed -n '2,9p' "$0"; exit 0 ;;
        *) echo "usage: ci/build.sh [--clean] [--test] [--debug]" >&2; exit 2 ;;
    esac
done

[ -n "$CLEAN" ] && rm -rf "$BUILD_DIR"
cmake -S "$REPO_DIR" -B "$BUILD_DIR" -DCMAKE_BUILD_TYPE="$BUILD_TYPE"
cmake --build "$BUILD_DIR" --config "$BUILD_TYPE" -j
[ -n "$RUN_TESTS" ] && ctest --test-dir "$BUILD_DIR" -C "$BUILD_TYPE" --output-on-failure
exit 0
