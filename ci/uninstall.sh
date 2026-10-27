#!/usr/bin/env bash
#
# ci/uninstall.sh — nothing to remove: ci/install.sh installs nothing.
# Removing nothing is success, so a loop over every repo's uninstall works.
set -euo pipefail
echo "nassau-dsp is header-only and consumed as a submodule: nothing to uninstall."
