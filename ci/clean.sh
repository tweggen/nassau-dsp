#!/usr/bin/env bash
#
# ci/clean.sh — throw away build output without throwing away the checkout.
#
#     ./ci/clean.sh      # build/ and the gate's build-ci/
#     ./ci/clean.sh -n   # print what would go, remove nothing
set -euo pipefail

REPO_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
DRY=""
case "${1:-}" in
    -n|--dry-run) DRY="yes" ;;
    "") ;;
    *) echo "usage: ci/clean.sh [-n]" >&2; exit 2 ;;
esac

for d in build build-ci; do
    if [ -d "$REPO_DIR/$d" ]; then
        if [ -n "$DRY" ]; then echo "would remove $d/"; else rm -rf "${REPO_DIR:?}/$d"; echo "removed $d/"; fi
    fi
done
