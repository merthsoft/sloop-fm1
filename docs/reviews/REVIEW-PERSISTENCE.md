# Persistence correctness review

Reviewed 2026-10-23, current working tree on `merthsloop` (base HEAD `9fd9ddc`, including uncommitted source). Review priority: correctness/ownership, then size/speed and maintainability. Production files and shared build outputs were not changed. Evidence is exclusive to `build/code-review-persistence-51/`.

## Integration recommendation

Fix F1 before release: a valid editor settings restore can interrupt active audio by erasing/programming flash. Fix F2 before treating successful saves as proof that favourites/theme/fork preferences reached flash. Both fixes can preserve the upstream payload/side-record layouts. No measured size optimization is proposed here; rebuild the target and check budgets after implementing the fixes. Existing icon/preset/headroom work is not counted again.

## Ranked confirmed findings

### F1 — P1: settings restore bypasses the transport guard when the song order is unchanged

**References:** `firmware/src/project.c:1124`, `firmware/src/project.c:1131`; caller `firmware/src/editor.c:483`; flash backend `firmware/src/felucca.c:106` and `firmware/hal/fm1_flash.h:249`.

`settings_restore()` rejects playback only when the incoming arrangement differs. A valid settings object containing the existing arrangement proceeds to `settings_write()` even with `song.playing` or `transport_req` set. Editor backup object 1 dispatches directly to this function before the `ed_flash_busy()` checks used for the other durable restore objects. This includes a backup taken from the same device: no malformed bytes are necessary.

**Trigger/consequence:** start playback, then restore that device's unchanged settings object (or modify only palette/lights). The path performs the settings sector erase and multiple program operations, calls `audio_silence()`, and disables interrupts in the flash driver. This violates the documented stopped-only flash rule and can produce an audible gap/missed audio deadlines. The finding is the reachable operation; no hardware gap duration was measured.

**Validation:** `restore_playing_repro.c` extracts the actual current `settings_restore()` body unchanged, supplies valid arrangement state and mock device/flash seams, and records `rc=0; writes=1` with `song.playing=1`. The harness uses a simplified persistence struct and permissive panel validator to isolate this branch; it is not a full protocol/hardware test. Actual production caller and backend were inspected to establish the complete path. Generation script: `make_repros.py`.

**Why tests miss it:** `project_test.c` defines `PROJ_HOST`, excluding the persistence/restore functions. `section_save_test.c` exercises real project saves but substitutes `settings_save()`. The editor-specific test covers drum-groove requests, not backup settings commit during playback. Web tests cannot establish firmware flash gating.

**Minimal fix:** reject `song.playing || transport_req` unconditionally before any settings restore mutation/write (rc 3), or guard object 1 with `ed_flash_busy()` before dispatch. Keeping the guard in `settings_restore()` also protects future callers. Remove the now-redundant arrangement-only playback gate. Do not defer silently while acknowledging a durable restore.

**Meaningful regression:** exercise actual backup begin/data/commit for object 1 with matching and different arrangements under playing and pending-start transport states. Assert rc 3, zero erase/program calls, and unchanged live settings/panel/arrangement. Repeat stopped and assert successful cold-boot restoration, including upstream settings bytes and preserved local fork preferences.

### F2 — P2: successful storage readback does not verify the side record

**References:** `firmware/src/storage.c:203`–`223`; success bookkeeping `firmware/src/project.c:1026`–`1028`; boot consumption `firmware/src/ui.c:831`.

`st_save3()` programs side bytes before the header, but the final verification checks only the header and payload. Side bytes are outside both the payload CRC and header CRC. A flash program operation which reports ready/success but silently fails to program the side area therefore passes verification. The driver checks write-enable and readiness; `fl_write()` does not perform a content comparison (`firmware/hal/fm1_flash.h:188`–`260`).

**Trigger/consequence:** a silent failed side-area program during a settings write. `settings_write()` then clears `fav_dirty` and records the desired fork settings as saved. At boot `side_take()` ignores missing/invalid favourites or theme data, while missing fork magic drops the local preferences. A successful UI save can thus lose favourites/theme/guide/swing/visualizer preferences after reboot, with no normal dirty retry. A later unrelated settings change may rewrite them, but that is not guaranteed.

**Validation:** `storage_side_repro.c` uses the current storage implementation and extends the existing NOR test with a program hook that returns success without programming offset 64. Existing storage assertions pass; the added check prints `save rc=0; side matches=0`. Full output: `storage_side.log`. This models a silent program failure, not an ordinary explicit program error or power cut; the latter are already covered and retain the prior copy.

**Why tests miss it:** `storage_test.c` injects failed program calls and checks torn-header fallback, but does not model successful-return/wrong-content side programming. Its side tests verify ordinary writes and header tears. Payload readback protects payload-only silent failures, which can obscure this separate omission.

**Minimal fix:** after programming the side bytes, read the same number of bytes from the candidate sector into the existing local side buffer and compare against `st_side`, before committing the header. Return failure on read or mismatch. Verifying before the header also retains the old committed copy as current. Do not alter the upstream settings payload length or reinterpret reserved settings bits.

**Meaningful regression:** save an old settings payload plus side record, silently drop/corrupt the next side program, and assert save failure plus old payload/side selection. Then retry without fault and assert both become current. Cover explicit error, read error, and no-side saves as separate cases. At settings level assert failed side writes retain dirty/retry bookkeeping and successful retry cold-loads the intended favourites and fork preferences.

## Coverage map and clean observations

| Area actually inspected | Assessment / limits |
| --- | --- |
| `storage.c`, flash hook wrappers and programming backend | A/B selection, payload CRC fallback, header-last commit, payload readback, noncontiguous autosave sectors, side area and XIP selection inspected. Ordinary torn writes, rot fallback and map separation have useful tests. F2 above. Sequence rollover was not stress-tested. |
| `project.c` codec and `project_save.c` | FUN1–FUN6 size/magic/hash dispatch, parameter migration, packed markers, clamped application, lock filtering, saved section versus working startup object, explicit save/retry queue and undo ownership inspected. A failed startup save restores the section dirty queue; subsequent retry captures the working state rather than substituting the section snapshot. Existing tests cover that distinction. |
| settings/panel persistence | PER1/PER2/PER3 branches, append-only lights boundary, MIDI-part lengths/default omission, SYN kit appendage, side prefix and local preference retention inspected. Restore validates panel permutations; boot panel initialization bounds IDs and directions. F1 above. Older-format migration branches were inspected but not independently reconstructed as flash fixtures here. |
| `upreset.c` and editor bank restore | Record and bank version/count checks, parameter mapping, parse bounds, name termination, physical user-bank addressing and XIP bank refresh inspected. Factory records remain separate from user banks. Record application clamps engine parameters. Host `UP_HOST` test uses RAM banks and does not exercise actual `up_put()`/XIP application ownership. |
| arranger state | `arranger.h`, `arranger.c`, `arranger_scene.c`, section save/load/erase and song backup/restore inspected. Scene snapshots are resident; callbacks do not perform flash operations. Arrangement validation checks active count/scenes/bars/availability; whole-project undo and song loop backup have separate storage. No new defect confirmed in those inspected paths. |
| editor serializers | Backup object lookup/list/get/begin/data/commit/abort, pack7/CRC/length handling at call sites, settings/MIDI/SYN/theme restore dispatch inspected; DSYN STORE uses stopped guard. Backup object 0 and user/FM6 staging share `proj_tmp`; other commands can invalidate the snapshot/staging lifetime, so protocol interleaving deserves a dedicated integration test. No silent corrupt commit demonstrated: PUT CRC normally rejects overwritten staging. |
| existing integrated work | Read `OPTIMIZATION.md`, `ARCHITECTURAL-HEADROOM.md`, `REDUNDANCY-AUDIT.md`; did not relabel integrated sharing or icon removal as new savings. |

## Test quality and follow-up opportunities (unmeasured)

- The independent upstream-produced FUN6 fixture test is stronger than a local codec self-roundtrip: it checks byte-identical import/apply/capture and specific arp/continuation/drum metadata. Preserve it when changing serializers. It remains one fixture, not proof of all upstream FUN6 combinations.
- The section-save test verifies snapshot versus subsequent live edits, real A/B flash, autosave-object failure and retry. Its settings seam deliberately cannot catch F1/F2 or actual persistence-layout errors.
- Highest-value new coverage is an actual firmware editor persistence harness with simulated NOR and realistic transport state, rather than more serializer assertions that mirror field assignments. Cover interrupted multi-object restore and failures of settings object parts independently.
- Test `up_put()` production behavior under failed erase/program and successful A/B pointer refresh. Its current deletion clears matching `trk[i].user` before the durable save result (`upreset.c:238`), so a failed deletion can alter live preset identity while the slot remains intact; confirm desired rollback behavior before treating this as a release-level defect. This is a lower-priority ownership inconsistency, not evidence of lost sound data.
- Document `proj_tmp` backup-transfer lifetime explicitly and test a bank save/project fetch between LIST/GET or PUT chunks; use an invalidation/generation check if needed rather than allocating another full project buffer without measurement.
- No cold-handler deduplication, flash/RAM savings, startup speedup or DSP speedup was measured in this review. Avoid generic frameworks or blanket inlining changes in these already coupled serialization paths.

## Validation and reproducibility

Private WSL build/run script: `build/code-review-persistence-51/run.sh`; compiler: existing `build/host/optimization-cc` (Zig 0.13), `-UNDEBUG -O1`, current source, output only in the private directory. `make_repros.py` creates the two private reproductions. The storage reproduction references the current production `storage.c` directly; the restore reproduction embeds the extracted function body. No candidate production patches were applied.

Test results are recorded in the private `*.log` files. See the completion note below for final pass/fail status.

## Explicit review limits

No hardware flash, power-cut rig, USB transport timing, target-size build or firmware publication was performed. No claim of absence of bugs is made. Most firmware tests compile selected source under host feature guards rather than the full production compilation unit; that is specifically material to the settings/backup gaps above. Musical audio goldens, factory content and compatibility were not modified. Concurrent reviewers may change unrelated working-tree files after inspection; references describe the inspected current source and should be rechecked during integration.

### Completion note

All eight freshly compiled current-source tests passed: storage, arranger, upreset, project, section_save, upstream_project, chord_voicing_storage and delay_storage. Both private reproductions also demonstrated the reported behavior. The test runner's final log-summary `tail` initially encountered a CRLF line-ending issue after all eight tests had completed successfully; the private script was normalized to LF. This affected summary display only, not compilation or test exit checks. Individual logs above retain the actual test outcomes.
