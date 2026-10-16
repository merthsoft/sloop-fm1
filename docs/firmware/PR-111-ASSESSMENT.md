# PR 111 assessment

The dual-core and low-RAM work is now integrated through upstream 2.7.1, together
with its safe core-hold fix. See the [integration record](UPSTREAM-2.7-INTEGRATION.md).
The staged assessments and measurements below are historical checkpoints;
hardware validation of the combined firmware remains pending.

Reviewed [the proposed dual-core/RAM series](https://github.com/isod89/sloop-fm1/pull/111)
against the current `merthsloop` branch. The independent MIDI routing, DRAWBAR arena,
main-loop scratch and XIP preset-bank reductions are integrated. Dual-core rendering
and low-memory relocation remain deferred.

The proposal is useful, but should be adapted in stages. Its author reports unchanged
golden audio renders and about 38 KB less ordinary static RAM on its own upstream base.
Those are upstream-series measurements, not savings measured on our fork.

## Integrated first stage

Adapted `fbafa34` from the PR: routing owners use packed nibbles, saving **1,024
bytes of static RAM** without reducing held-note capacity. Our scale-aware MIDI
ownership, USB/TRS separation and selected-track release routing are retained.
All routing-table callers and test fixtures use the packed accessors.

The native scale/MIDI ownership and musical-starter regressions pass. With the
STEP held-help fix included, the target build reports 93,476 bytes of static RAM
(previously 94,500), unchanged pool usage of 333,900 bytes, and an application
image of 581,356 bytes. These are measured fork results, not the PR's total savings.

## Additional integrated reductions

Adapted `904d386`, `d024b41` and `72300ea` with the fork's existing arenas, visualizer
histories, project recovery and flash-driver reserve intact:

- DRAWBAR shares the per-track engine arena; compile-time size/index checks protect
  the layout. Its wheel state resets after an arena engine takes over, as intended.
- Storage payload and decoded OTA frames share main-loop scratch. OTA staging uses
  `proj_tmp` during its blocking session. Scope/FFT real input also shares scratch,
  saving 1,024 pool bytes. Existing `.scratch` visualizer histories remain separate.
- User-preset banks are read from the flash XIP window. Writes stage separately in
  `proj_tmp`, and pointers are refreshed after saves and editor restores. Empty,
  corrupt or absent-flash banks remain empty; absent flash reports NO FLASH.

The pure additional memory pass measures **11,440 fewer static RAM bytes** and
**1,024 fewer pool bytes**. Together with MIDI packing, the reductions recover
12,464 static bytes before new runtime effects. MerthSloop 2.6.0.11 with wah/pan
uses 82,100 static bytes and 332,876 pool bytes; its image is 581,420 bytes.

Full host tests pass: unchanged 182 golden audio renders, expanded DRAWBAR takeover
and random-switch tests, XIP A/B/torn-write/corruption tests, storage, OTA/loader,
project recovery and 20,000-frame UI fuzz. Hardware timing/flash-cache validation
is still a physical-device check.

## Deliberately deferred

The `.lowram` relocation and dual-core renderer are **not** integrated. We already
use spare flash-driver RAM under a reserve check; transplanting that memory map
would overlap existing state. Dual-core also needs separate interrupt/order,
stack high-water and timeout-fallback hardware validation. These are not required
for the independent savings or the new wah/pan effects.

## Original staged assessment

1. Pack `midi_sel_on` into nibbles. Values are 0–4; this reduces the routing table
   by 1,024 bytes. Our held-note/latch logic also reads the table, so adapt every access
   and rerun routing, sustain, latch and disconnect tests.
2. Move DRAWBAR state into the existing per-track engine arena. The PR reports about
   2.5 KB savings; wheel phases reset after switching through another arena engine.
   Retain explicit arena-size checks and verify repeated engine/preset changes.
3. Adapt XIP user preset banks and shared main-loop scratch separately, checking
   pointer invalidation after A/B writes, no-flash behavior, and scratch lifetimes in
   our backup, preview, undo, scene and visualizer paths. The PR's shared-scratch
   change adds code, which matters with our limited application margin.
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
