# Compact parameter descriptors

## Retained change

A parameter descriptor previously stored label and names pointers, four bytes of range/format/default data, and a separate unit-string pointer. Most unit pointers were null. The target struct was 16 B. Removing the unit pointer makes it 12 B, also shrinking the eight embedded edit descriptors in each engine table.

Five internal integer variants encode the explicit units ct, st, MS, percent and Hz. param_unit recovers the unit; param_fmt normalizes these variants to the original F_INT. Named integer ranges still work alongside a unit (notably INSERT CRUSH RATE). The original public format IDs are unchanged. The actual ED_DESC handler uses the normalized format and recovered unit, so the editor/Android protocol receives precisely its previous bytes.

All ordinary parameter descriptors and engine edit descriptors use the compact structure. Saved parameter IDs, ranges/defaults, preset recipes/IDs, project files, factory audio and sample data are unchanged. There is no startup expansion, resident cache, feature removal or new audio-loop lookup. Unit interpretation runs in UI formatting/editor descriptions. Hot consumers retain the existing names/range fields.

## Actual measurements

| Source | Image B | Free B | Main / low / pool static RAM B |
|---|---:|---:|---|
| Frozen .103 working baseline, including SONG SAVE fix | 583,912 | 424 | 97,724 / 29,288 / 335,424 |
| Compact descriptors, same .103 version | 582,936 | 1,400 | unchanged |
| Final .104 package | 582,936 | 1,400 | unchanged |

The net saving is **976 B** in the complete pinned target link, not a sum of individual table sizes. The 584,336 B application limit and all RAM limits/reserves are unchanged. Main has 580 B free; low RAM 104 B; the pool retains its 8,192 B reserve plus 448 B. The current -Oz and protected application-only threshold-zero policy remain unchanged. No DSP filter-sharing candidate was added: the earlier register/stack trade remains deferred.

A separate semitone-formatting cleanup was measured and rejected: moving a redundant positive-value fmt_int call into the else arm increased the link by 16 B. The smaller original form is retained.

## Independent equivalence checks

Before editing, .103 firmware/tests/generated inputs and linked outputs were frozen under ignored build/headroom-104/reference. Original canonical metadata includes globals, track controls, engine descriptors across every factory preset, insert modes, synced envelopes and ambient/morph controls. Every displayed in-range value and unit was emitted. Actual ED_DESC requests were separately captured for all track/global controls across all factory presets.

- Canonical descriptor/value/unit stream: **1,160,878 B**, exactly equal to original .103 output.
- Actual editor response stream: **449,269 B**, exactly equal to original .103 output; internal unit format IDs never reach the wire.
- Maintained checks: descriptor_oracle_test.c, descriptor_wire_test.c and parameter_descriptors_test.py. The small fixture stores SHA-256 and byte counts from the original implementation; the normal test cannot regenerate expectations.
- **79 full framebuffer captures** compare byte-for-byte to the frozen .103 renderer, with full navigation and 20,000-frame clipping/fuzz checks passing.
- All **210 existing audio goldens** remain unchanged; health, voice/routing and host cost checks pass without updating expected audio.
- Project/upstream byte round trips, real NOR SONG SAVE/cold restart, A/B snapshot persistence/interpolation, parameter acceleration, sound-browser mapping, LOFI, reverse-delay and dual-core mixer tests pass.
- Target audio-loop instruction/divide/call/weighted-cost metrics are all **identical** to the frozen .103 disassembly; original budget and 25-function inlining guards pass. Master LOFI retains zero sample-loop divides/calls.
- Actual final literal restoration/pointer oracle, startup-restored preset records and native/WASM audio parity accompany the packaged build.

Evidence lives under ignored build/headroom-104. The frozen comparisons demonstrate behavior preservation for the exercised controls; they are not new hardware timing measurements. No device was flashed automatically.
