# nassau-dsp

Shared, header-only DSP primitives of the Nassau family: the filters and
shapers that more than one instrument uses. Standard C++17 headers only — no
Qt, no IPlug2, no plugin SDK — so any Nassau DSP core, and qbx through
NassauDigital's core, can build it from one checkout.

| Header | What | From |
|---|---|---|
| `Source/DSP/synth_dsp.h` | `nassau_real`, `nassau::kControlBlock` (the 32-sample control-block contract), `TptOnePole`, `OnePoleHP`/`OnePoleLP`, `SmoothedValue`, `ScopedNoDenormals`, `shapeTriodeK`, `shapeCubic`, `Xorshift32`, `PinkFilter`, `polyBlep`, `AdsrEnv`, `Lfo` | nassau-analogue (and nassau-zermatt before it) |
| `Source/DSP/synth_filter.h` | `LadderFilter` (24 dB/oct ZDF low-pass, saturating feedback), `SvfFilter` (12 dB/oct low-pass), `HpfCascade` (non-resonant 12/24 dB/oct high-pass) | nassau-analogue |

Both moved out of nassau-analogue unchanged in code for NassauDigital's gate
X1 (nassau-digital `docs/GATES.md`); their comments still cite
nassau-analogue's design sections, where each was decided and gated.

## Using it

nassau-dsp is pinned **once** for a release, as a top-level submodule of
nassau-suite, which passes `-DNASSAU_DSP_DIR` to every component. A consumer
never carries it as a nested submodule (a second pin is a second answer to
which DSP shipped). Instead it copies the locator
[`cmake/bootstrap/NassauDSP.cmake`](cmake/bootstrap/NassauDSP.cmake) verbatim
to its own `cmake/NassauDSP.cmake` and does:

```cmake
set(NASSAU_DSP_GIT_REF "<commit sha or tag>")   # the bare-clone fallback
include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/NassauDSP.cmake)
target_link_libraries(my_core PUBLIC nassau::dsp)
```

It resolves, in this one order: an explicit `-DNASSAU_DSP_DIR`, a sibling
checkout `../nassau-dsp`, then a FetchContent of `NASSAU_DSP_GIT_REF`, and logs
which one and at which commit.

Tests build only when nassau-dsp is the top-level project. Two definitions
change what the headers compile to and must be set directory-wide by the
consumer, so every translation unit agrees: `NASSAU_DSP_FLOAT` (filter state
in `float`) and `NASSAU_NO_SIMD` (read by nassau-analogue's SIMD path).

## Gate

```bash
./ci/gates.sh     # header check (R2) + build + tests, the same command CI runs
```

`ci/build.sh`, `ci/clean.sh`, `ci/install.sh` and `ci/uninstall.sh` are the
four verbs every Nassau repo has; install and uninstall are no-ops for a
header-only library.
