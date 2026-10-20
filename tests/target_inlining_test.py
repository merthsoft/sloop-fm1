#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Guard target per-sample helper inlining under size compilation.

Caps are the merged 2.6 -Os build's call sites inside backwards-branch loops,
measured with the pinned JieLi compiler. They protect hot DSP from -Oz outlining
small helpers without conflating branch layout with the historical cost budget.
This is a static call-site check, not a hardware cycle or timing measurement.
"""
import sys
from pathlib import Path
from target_budget import functions, cost

LOOP_CALL_CAPS = {
    "analog_render": 0, "digital_render": 0, "phase_render": 2,
    "lofi_render": 0, "sample_render": 1, "formant_render": 0,
    "trio_render": 2, "trio_pass": 0, "drawbar_render": 0,
    # Raw samples have no cache-miss decoder calls. SLICER's swing call is per step.
    "drawbar_block": 0, "grain_render": 1, "grain_block": 1,
    "slicer_track": 1, "drums_mix": 1, "fm6_op_run": 0,
    "fm6_op_fb": 0, "fm6_render": 6,
    "fm1_alnk0_irq": 4,
    "phys_render": 0, "noise_render": 0, "px_modal_run": 0,
    "px_string_run": 0, "px_string_excite": 0, "px_symp_run": 0,
}

def main():
    path = Path(sys.argv[1] if len(sys.argv) > 1 else "build/felucca.dis")
    found = functions(path)
    failed = []
    for name, cap in LOOP_CALL_CAPS.items():
        if name not in found or not found[name]:
            failed.append(f"{name}: target symbol missing")
            continue
        calls = cost(found[name])["call"]
        if calls > cap:
            failed.append(f"{name}: {calls} loop call sites exceed the -Os cap {cap}")
        print(f"target inline: {name}: {calls} loop calls (cap {cap})")
    for error in failed:
        print("FAIL: " + error)
    if not failed:
        print(f"target DSP inlining: PASS ({len(LOOP_CALL_CAPS)} hot functions)")
    return bool(failed)

if __name__ == "__main__":
    sys.exit(main())
