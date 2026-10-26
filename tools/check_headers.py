#!/usr/bin/env python3
"""R2 for nassau-dsp: every header under Source/DSP/ includes only the
standard headers below, the x86 intrinsic header (guarded in the source on
MSVC/x86), or a sibling header of Source/DSP/ itself.

A consumer (nassau-analogue, NassauDigital's core, and through it qbx) takes
these headers into a framework-free build. One stray <QtCore> or IPlug2
header here would put that framework into every one of them, so the rule is
a gate, not a convention.

    python3 tools/check_headers.py          # exit 0 clean, 1 on a violation
"""
import pathlib
import re
import sys

ROOT = pathlib.Path(__file__).resolve().parent.parent
SRC = ROOT / "Source" / "DSP"

# Standard library headers the DSP may use. Deliberately short: widening it
# is a reviewed change to this file, not something a header does by itself.
ALLOWED_SYSTEM = {
    "algorithm", "array", "atomic", "cmath", "cstddef", "cstdint", "cstring",
    "limits", "type_traits", "utility",
    "xmmintrin.h",  # MXCSR flush-to-zero in ScopedNoDenormals, guarded on x86
}

INCLUDE = re.compile(r'^\s*#\s*include\s*([<"])([^>"]+)[>"]')


def main() -> int:
    siblings = {p.name for p in SRC.glob("*.h")}
    bad = []
    for header in sorted(SRC.glob("*.h")):
        for n, line in enumerate(header.read_text().splitlines(), 1):
            m = INCLUDE.match(line)
            if not m:
                continue
            kind, name = m.groups()
            ok = (kind == "<" and name in ALLOWED_SYSTEM) or (kind == '"' and name in siblings)
            if not ok:
                bad.append(f"{header.relative_to(ROOT)}:{n}: {line.strip()}")
    if bad:
        print("check_headers: FAIL - includes outside R2's list:")
        for b in bad:
            print("  " + b)
        return 1
    print(f"check_headers: PASS ({len(siblings)} headers)")
    return 0


if __name__ == "__main__":
    sys.exit(main())
