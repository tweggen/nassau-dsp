#!/usr/bin/env bash
#
# ci/install.sh — nothing to install: nassau-dsp is a header-only library that
# its consumers take as a git submodule and add with add_subdirectory().
#
# It exists because every Nassau repo answers to the same four verbs (build,
# clean, install, uninstall), so a loop over the repos never has to special-
# case one. It succeeds and says so.
set -euo pipefail
echo "nassau-dsp is header-only and consumed as a submodule: nothing to install."
