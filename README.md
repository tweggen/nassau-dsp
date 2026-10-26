# nassau-dsp

Shared, header-only DSP primitives of the Nassau family: the filters and
shapers that more than one instrument uses. Standard C++17 headers only — no
Qt, no IPlug2, no plugin SDK — so any Nassau DSP core, and qbx, can build it
from one checkout.

The first content arrives with NassauDigital's gate X1 (nassau-digital
`docs/GATES.md`): the 24 dB/oct ladder, the 12 dB/oct SVF, the high-pass
cascade and `shapeTriodeK`, moved out of nassau-analogue.
