# PR 111 assessment

Reviewed [the proposed dual-core/RAM series](https://github.com/isod89/sloop-fm1/pull/111)
against the current android branch. No commits from it are integrated by this assessment.

The proposal is useful, but should be adapted in stages. Its author reports unchanged
golden audio renders and about 38 KB less ordinary static RAM on its own upstream base.
Those are upstream-series measurements, not savings measured on our fork.

## Suggested order

1. Pack `midi_sel_on` into nibbles. Values are 0–4; this reduces the routing table
   by 1,024 bytes. Our held-note/latch logic also reads the table, so adapt every access
   and rerun routing, sustain, latch and disconnect tests.
2. Move DRAWBAR state into the existing per-track engine arena. The PR reports about
   2.5 KB savings; wheel phases reset after switching through another arena engine.
   Retain explicit arena-size checks and verify repeated engine/preset changes.
3. Adapt XIP user preset banks and shared main-loop scratch separately, checking
   pointer invalidation after A/B writes, no-flash behavior, and scratch lifetimes in
   our backup, preview, undo, scene and visualizer paths. The PR's shared-scratch
   change adds code, which matters with our current 72-byte application margin.
4. Reconcile `.lowram` with our existing `.scratch` placement and enforced flash-driver
   reserve before dual-core work. Both use SRAM below the ordinary RAM region;
   their reported savings cannot simply be added to ours.
5. Integrate dual-core rendering as a dedicated change. Audit our held vibrato/tremolo,
   sequenced arps, source velocities, note capture, MIDI input and preset transitions
   alongside the PR's RNG/order, envelope sync and per-core render scratch protections.

## Review limits and checks

`git apply --check` fails on app.ld, panel.c, seq.c, ui_draw.c, ui_menu.c, ui_vis.c,
run_tests.sh and tools/build.py. A direct cherry-pick/patch is not a safe integration.

The timeout fallback holds core 1 and rerenders its units on core 0; the PR comments
acknowledge a glitch if the timed-out job already partially advanced state. Its 8 ms
timeout must be evaluated against our audio deadlines. Golden output tests and host
race checking are useful but do not substitute for worst-case physical timing,
save/flash/reboot safety and stack high-water checks on the FM1.

Recommendation: take the small independent RAM reductions first, then memory placement
and dual-core separately. Rebuild and measure actual ROM/RAM deltas at each stage.
