# Sequencing review â€” current working tree

Reviewed 2026-10-23, branch `merthsloop`, HEAD `9fd9ddc`, including the working tree rather than HEAD alone. Production files were not changed. Evidence and a frozen source/test/generated-header snapshot are exclusively under `build/code-review-sequencing-51/`. No target image, shared generated file, Git ref, or other review report was written. No repository AGENTS.md was found in the workspace or its inspected ancestors.

## Original review integration recommendation

Fix the four confirmed ownership/reconciliation defects below before treating the sequencing review as complete. The existing scheduler and test harnesses provide suitable foundations; a scheduler rewrite or generic ownership framework is unnecessary for the editor notification and arp MIDI fixes. Keep fixes separate and target-build each accepted change. This review establishes no additional flash headroom and does not re-count work in OPTIMIZATION.md, ARCHITECTURAL-HEADROOM.md or REDUNDANCY-AUDIT.md.

## Ranked confirmed findings

### 1. [P2] Phone step writes bypass live-ratchet reconciliation

**Locations:** `firmware/src/editor.c:253-276` (`ed_step_in`), callers at 778 and 1039; `firmware/src/seq.c:2788-2801`. The existing native safeguard is `seq_edit_changed` at `seq.c:2628`.

**Trigger/consequence:** Start a four-note direct synth step with a ratcheted note, then change that note's pitch using ED_STEP_SET or ED_TRACK_STEP before a subsequent ratchet. The write changes the stored step but does not set `seq_skip`. `seq_ratchets` attacks the edited pitch, but the old four-note in-flight list is full, so the new pitch cannot be appended. On the next REST, `seq_release` releases only the old list. The new MIDI note stays on until a track/global cleanup (STOP does clean it). This can also leave the corresponding audio gate outside sequence ownership.

**Reproduction:** `snapshot/tests/repro_editor.c` includes production hostsim and an exact extracted copy of `ed_step_in` from the frozen editor source; the drum conversion stub is unused. It starts `[60,64,67,71]`, edits 60 to 72 through the wire helper, executes a later ratchet, then plays REST. `repro_editor.log`: `after REST seq_n=0 MIDI72=1 seq_skip=0`. This is a production-helper/scheduler reproduction, not a full SysEx decoder test; both decoder callers were inspected and have no other reconciliation call.

**Minimal fix:** In the existing IRQ-protected mutation transaction, call `seq_edit_changed(t)` when writing a synth step (prefer the shared helper so both commands are covered). Preserve the existing next-step reconciliation policy. Consider the drum edit path separately: it also rereads the active cell, but it does not have the synth four-pitch ownership failure.

**Why tests miss it / regression:** `tests/live_note_editing_test.c:60-66` drives native `step_edit`, which already sets `seq_skip`; no test found invokes `ed_step_in` or these two editor commands. Add wire-command tests for selected and explicit tracks, edit each of four occupied slots between ratchets, advance through gate expiry and REST, and verify per-channel/pitch note balance *before* STOP. Also verify another track keeps its notes and the edited pitch plays on the next loop.

### 2. [P2] Arp chord revoicing overwrites the raw-key MIDI release pitches

**Locations:** `firmware/src/seq.c:1935-1970`, particularly 1952-1964; raw note-ons at 2192 and note-offs at 2248-2256.

**Trigger/consequence:** Enable chord mode and arp, hold a root, press a chord modifier, release the root while the modifier remains down, then release the modifier. The arp branch updates the live arp pool and overwrites `kb_nt[k]` with the modified chord, but emits no corresponding raw-key MIDI off/on changes. Key-up consequently sends offs for the modified chord instead of the original notes sent on. An external instrument retains the original changed tone and receives an unmatched off for the replacement tone. This happens even with G_MIDI=KEYS, independent of generated-note overlap.

**Reproduction:** `snapshot/tests/repro_review.c`, argument `arp`, drives real `events_block` key edges: key 7 (C chord), modifier key 5 (sus4), root release before modifier release. `repro-revoice.log` shows channel 1 note 64 balance +1 and note 65 balance -1 after all releases and STOP. Generated output was deliberately disabled to isolate the always-sent raw key stream.

**Minimal fix:** In the physical-key part (`k < 27`) of the arp revoice branch, reconcile raw MIDI lost/gained tones before replacing `kb_nt[k]`, following the existing direct-voicing difference loops. Do not emit raw key notes for the arp latch-only entry (`k == 27`), whose physical release has already sent its raw offs. Pair this fix with finding 3's channel ownership handling.

**Why tests miss it / regression:** The physical loop in `tests/chord_modifiers_test.c:58-71` restores the momentary modifier before finally releasing the root, or toggles latched modifiers back. Thus its final balanced-MIDI assertion sees the original release list again. Add the opposite release order for every modifier, arp on/off and HOLD on/off, with KEYS and KEYS+SEQ output. Check balance after each gesture, not only after the entire modifier sweep.

### 3. [P2] Changing MIDI channel while a physical key is held strands its original outgoing note

**Locations:** `firmware/src/seq.c:2183-2192`, 2248-2256; drum equivalent at 2142-2143 and 2262-2263. Channel-change cleanup is at 3490-3513 (the `chans != chans_was` block).

**Trigger/consequence:** Hold a physical key on CH1=1, change CH1 to 2, then release. The channel change requests audio panic and calls `seq_out_all_off`, which only knows generated `mo_set` notes. The raw physical key note was sent directly and is not in that set. Key-up re-reads the new channel and sends its off there. The old channel remains on even after STOP. The same pattern exists for physical drums and for direct chord latch cleanup, which derives its channel from current settings.

**Reproduction:** `snapshot/tests/repro_review.c` without arguments uses the real keyboard/events path and channel-change panic. `repro-channel.log`: channel 1 note 60 balance +1, channel 2 note 60 balance -1 after release and STOP.

**Minimal fix:** Preserve the actual outgoing channel for each physical-key owner (and transfer that channel when ownership becomes a direct chord latch), or retire raw owners explicitly on their old channel during a channel-change transaction before adopting the new routing. Merely calling `seq_out_all_off` again cannot fix notes absent from `mo_set`. Use the same remembered channel during modifier reconciliation and key/drum release. Account for the small extra runtime state in the target RAM budget if choosing remembered channels.

**Why tests miss it / regression:** Generated-note tests exercise remembered `mo_ch` and routing changes; the inspected physical modifier tests keep channels stable. Add synth, drum, chord and latched-key cases that change channels with keys held, including channel OFF. Drain and assert counts by `(channel,pitch)` across the change and release. Include preset panic and source takeover so cleanup cannot silently discard the release identity.

### 4. [P2] Releasing one overlapping arp chord removes a pitch still owned by another physical key

**Locations:** `firmware/src/seq.c:1226-1231` deduplicates live membership; 1248-1262 removes membership unconditionally by pitch. Physical releases call it through `input_off_now` at 1692.

**Trigger/consequence:** With chord mode, arp ON and HOLD OFF, hold C then G. Their pitch sets share G4 (67). The pool correctly becomes the union `[60,64,67,71,74]`, but releasing C removes 67 even though the still-held G chord owns `[67,71,74]`. Subsequent generated cycles omit the remaining chord's root. The total `arp_phys` counter cannot represent per-pitch ownership. This is not the intentionally exclusive progression pad mode: these are ordinary simultaneously held physical chord keys.

**Reproduction:** `snapshot/tests/repro_overlap.c` drives physical keys 7 and 14 through `events_block`; `repro_overlap.log` shows the union before release, remaining-key pitches `[67,71,74]`, and the erroneous pool `[71,74]` after releasing key 7.

**Minimal fix:** Preserve shared live pitch ownership on release. For the physical-only case, consult remaining physical chord owners before removing their shared pitch, while still decrementing the physical press count correctly. If implementing per-pitch reference counts instead, audit MIDI and sustain callers and source-replacement operations as well; a count alone does not preserve each owner's `held_key`/`held_dt` provenance for subsequent scale changes. Avoid a broad owner-system replacement without separate behavioral tests.

**Why tests miss it / regression:** Current arp overlap coverage focuses on live-versus-sequence ownership and octave-expanded velocity collisions; modifier tests hold one chord root. Add C+G and other shared-tone chord pairs, both release orders, and modifier changes while both roots are held. Assert the exact source union and generated cycles after the first release, then an empty pool after the second. Include octave/scale remapping so the remaining owner supplies valid provenance.

## Coverage map and designs retained

The actual compilation graph is `felucca.c -> seq.c -> harmony_owners.h, harmony_voicing.h, note_ties.c`. Arp, recording, key/chord latch, progression, roll, transport, MIDI routing and sequence timing implementations are inside seq.c, not separate compiled modules. `felucca.c -> ui.c -> rhythm_shapes.c, drum_grooves.c, sequence_starters.c`; `starter_pattern.h` is guarded and shared by the two starter libraries. `felucca.c -> ui_sequence_starters.c` supplies progression chord construction and preview bindings. `felucca.c -> ui_input.c -> ui_note_actions.c -> note_chain_actions.c`, and ui_input.c also includes ui_step_move.c. `editor.c` includes editor_performance.c and editor_drum_grooves.c. `fx.c -> punch.c -> performance_remote.h` supplies the bounded performance owners consumed by seq.c.

Inspected:

- seq.c timing grids, swing/early-late scheduling, ratchet gate ownership, per-note tie lookahead/slot compaction, sequence route changes, key and MIDI input, sustain, latch/modifier differences, octave provenance, progression source replacement and direct/arp transitions, recording quantization/hold/end behavior, transport and clock source selection.
- note_ties.c and note_chain_actions.c mutation/collision/capacity paths; native mutation callers in ui.c, ui_input.c, ui_layers.c, ui_note_actions.c and ui_step_move.c. Editor step and groove/performance mutation semantics were read; screen rendering was excluded.
- harmony_voicing.h usage, core.h division/swing helpers and relevant track capacities, sequence starter render/add/apply/preview paths, rhythm_shapes.c, drum_grooves.c, starter_pattern.h, performance_remote.h and editor_performance.c.
- Tests listed below plus relevant source assertions in live_note_editing_test, progression_perform_test, swing_grid_test, chord modifier, tie and chain tests.

Good existing choices worth preserving: sequence harmony is distinct from live HOLD membership; literal sequence pitches bypass keyboard quantization; route transfers carry sounding velocities; independent ties follow pitch identity rather than slot identity and compact velocities with gates; recording skip does not suppress sequencer-fed harmony; generated notes use remembered outgoing channels; previews yield before new live key attacks; note-chain operations validate capacity/collisions before writing and preserve lock/micro metadata. The all-note recording hold policy and additive starter shared-attack policy are documented intentional constraints, not reported as defects.

## Validation and test quality

All compilation/execution used the frozen private tree and Zig 0.13 host compiler with `-O2 -UNDEBUG`; outputs/logs remain private. The focused run passed seq_note_ties_test, seq_arp_test, seq_arp_modes_test, seq_arp_expression_test, seq2_test, note_chain_actions_test, sequence_starters_test, performance_engine_test, chord_modifiers_test, progression_perform_test, live_note_editing_test and swing_grid_test. The swing harness initially failed to open a PPM because its default private build/host directory was absent; creating that directory and rerunning the unchanged binary passed. The three defect reproductions and overlap trace above also ran. The editor reproduction extracts an unchanged helper rather than claiming whole-editor coverage.

The stronger tests exercise production voice/MIDI scheduling and assert intermediate gate identity or unchanged bytes on rejected transactions. In particular, independent-tie voice-age assertions and note-chain negative-result byte comparisons test useful invariants. The randomized chain model still calls production `note_effective` and `note_chain_len` to construct its expectations; it can miss a shared error in legacy-tie expansion/chain boundary interpretation. Add a small independently specified event-timeline oracle for mixed legacy/independent ties rather than simply increasing its random iteration count. A final STOP balance assertion can hide an untracked note that sounded for several REST steps; check balance at each expected silence boundary. Some older arp/tie drains index only pitch, so they cannot validate channel identity; retain the per-channel style used in newer modifier/progression tests.

## Size, speed and duplicate primitives

No new measured size/speed optimization is claimed. Arp pool construction/sorting/expansion is bounded (16 roots, 64 expanded pitches), and generated PULSE output is bounded by the 64-entry generated list. Sequence timing plays at most one new step per block. Those are source bounds, not ISR cycle measurements. Saturated overlapping PULSE processing contains bounded repeated scans, but no target measurement demonstrates an actionable regression or useful saving here.

No additional pair of duplicate control primitives was proven interchangeable. Whole-step editing, independent-note chain editing, live recording extension, arp membership and progression replacement have different ownership/metadata semantics. Existing shared rhythm/metadata helpers and generated_emit already remove real overlap; do not collapse these into a generic framework or re-count earlier audit savings. Preserve explicit cold/noinline decisions already justified in optimization documentation and DSP_INLINE hot helpers.

## Explicit limits

This is a focused source and host-behavior review, not proof of absence of other bugs. No JieLi target rebuild, target byte delta, ISR latency measurement, electrical MIDI/USB timing test, flash operation, publication or hardware audition was performed. Hostsim disables target attributes and its IRQ stubs cannot reproduce actual interrupt races. Mutation transactions were traced for exclusion/notification, but volatile/source ordering and real concurrent interrupt behavior were not proven. Factory content and persistence compatibility were not changed. The complete repository suite, audio goldens and sanitizers were not rerun. Recommendations must preserve the existing sample representation, partition boundaries, saved formats, protocol behavior and audio deadlines.



## Implementation follow-up â€” SEQ-1 through SEQ-4

Implemented in the authorized shared working tree after parent approval on 2026-10-23. This implementation edited only `firmware/src/seq.c`, `firmware/src/editor.c`, the exclusive sequencing tests/runner, and this report. No commits, version bumps, shared generated/build outputs, publication, firmware packages or hardware writes were performed. The findings above remain the historical diagnosis; the four assigned fixes are now implemented.

### Changes

- **SEQ-1:** The shared synth `ed_step_in` decoder calls `seq_edit_changed` inside its callers' existing IRQ-excluded transaction. Both selected-track STEP_SET and explicit TRACK_STEP now use the native policy: finish the in-flight pitches, suppress rereading an edited cell for ratchets, and adopt new content at the normal playback boundary. Drum semantics are unchanged.
- **SEQ-2:** Arp revoice emits raw MIDI offs for lost physical-key tones and ons for gained tones before overwriting their release list. The latch-only arp entry emits no raw MIDI because its physical release already ended that stream.
- **SEQ-3:** Raw synth/drum keys remember their press channel; direct chord latches inherit it. Revoices, key/drum releases, KBD HOLD transfer, direct latch cleanup and preset-fade latch cleanup use the remembered channel. The existing route/channel-change transaction retires physical note/drum owners through their original release path before panic clears audio/latch state. Later physical releases are inert. This includes channel OFF and prevents unmatched offs on a newly selected channel.
- **SEQ-4:** Before deleting a deduplicated live arp pitch, `arp_remove` checks remaining physical note owners. If one still owns it, the pitch stays and its `held_key`/`held_dt` provenance moves to the survivor, preserving press octave and subsequent scale/root remapping. Revoice supplies the existing `in_key` context to exclude the replaced owner. The aggregate physical count still decrements once per removed press; HOLD retains its existing latch policy. No generic owner pool, persistence/protocol field or audio buffer was introduced.

The added ownership check runs on note-removal/revoice events, scanning the existing bounded 27 physical owners (at most four tones each) and existing 16-tone live pool. It is not in a per-sample render path. No measured hardware ISR latency claim is made.

### Regression evidence

`tests/sequencing_ownership_test.c` uses real `events_block` edges, modifiers, preset changes, progression takeover, direct/arp transitions and outgoing MIDI packets. The oracle indexes channel/pitch and rejects unmatched offs. Coverage includes all eleven modifiers; both root/modifier release orders; direct/arp, HOLD on/off and raw-only/generated output; synth/drum/chord/KBD HOLD channel changes including OFF; C+G/C+E shared-tone release orders; retained press octave and scale/root remap; overlapping modifier edits; preset panic and progression direct/arp takeover. Assertions occur at release boundaries before STOP. Original-source cases 2, 3 and 4 each aborted on the expected balance/shared-pitch assertion; all cases pass after the fixes.

`tests/sequencing_phone_step_test.c` compiles the full production editor and calls actual `ed_handle` STEP_SET/TRACK_STEP dispatch, rather than extracting the helper. Flash writes and unrelated editor musical hooks assert if invoked. It edits each of four occupied slots while ratchets run and lookahead ties keep four old pitches owned, verifies no edited pitch appears in-flight, then verifies REST release, next-loop playback and another track's held tone. The original editor aborts at the intermediate channel/pitch assertion; the fixed editor passes all eight command/slot cases. This tests command dispatch/decoding, not USB SysEx byte transport.

The first phone-test draft allowed its gate to expire before the ratchet and did not expose full capacity. The final test keeps four tied pitches owned and fails on the original editor. That earlier draft is not counted as reproduction evidence.

Logs are in `build/code-review-sequencing-51/SEQ-{1,2,3,4}-before.log`, `SEQ-{1,2,3,4,5}-after.log`, and `current-working-tree-tests.log`. Both new tests also passed against the **current shared production sources**, not only a frozen snapshot. `tests/run_sequencing_ownership.sh` runs both tests with outputs defaulting to the private sequencing directory. The parent owns wiring the shared runner; `tests/run_tests.sh` was not edited here.

### Existing tests and target validation

All twelve existing focused tests passed with the fixes: seq_note_ties_test, seq_arp_test, seq_arp_modes_test, seq_arp_expression_test, seq2_test, note_chain_actions_test, sequence_starters_test, performance_engine_test, chord_modifiers_test, progression_perform_test, live_note_editing_test and swing_grid_test. Logs are `fixed-*.log`. They used a consistent frozen source/header tree with current assigned seq.c/editor.c overlaid. An initial live-tree compile caught a transient concurrent preset-trim source/header mismatch; the private tree avoided interference. Both new tests subsequently passed against the live shared tree once its representation was consistent.

Private before/after application-only links used pinned JieLi Clang 4.0.1, normal `-Oz` build_app flags, private preset-generated data and unchanged baseline sources outside the assigned files. Package generation was not invoked. Both links pass `build.check`, including placement, RAM/pool budgets, RAM-code calls and ROM-reference checks:

| Metric | Before | After | Delta |
| --- | ---: | ---: | ---: |
| Application image | 581,296 B | 580,784 B | âˆ’512 B |
| Main RAM data+BSS | 89,128 B | 89,160 B | +32 B |
| Low RAM | 28,280 B | 28,280 B | 0 |
| Audio pool | 332,876 B | 332,876 B | 0 |

Explicit new state is 30 bytes (27 key channels plus three latch channels), with alignment accounting for the linked 32-byte RAM increase. The image reduction is a measured whole-link compiler/layout result, not an independently additive optimization promise. Concurrent changes can alter it; parent must use the final combined target result. Evidence: `target-results.json`, `target-measure.log`, `target-{before,after}-symbols.txt` and private `target/` sources/outputs.

### Handoff and limits

All four assigned fixes are ready for parent integration. No production hook outside this ownership is required. Parent should include both new tests in the normal runner and validate the final combined target. Host IRQ stubs cannot prove races/deadlines; no hardware MIDI audition occurred. The full repository suite, audio-golden set and sanitizers were not rerun by this implementation. The physical shared-pitch repair does not replace or prove every legacy live MIDI/sustain same-pitch policy. Factory content, saved formats and upstream interfaces are preserved.
