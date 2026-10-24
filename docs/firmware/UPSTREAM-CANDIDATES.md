# SLOOP 2.8 integration candidates

Assessment: 2026-10-21. Recommendation: integrate selected fixes in stages after the active reverb work settles; do not merge the release wholesale into MerthSloop yet. Difficulty below is a source-review estimate, not a measured implementation schedule.

## Current resolution

The selected candidates are implemented through the 2.8-based build. Conflicting
navigation, sample cuts, stock-return removal and USB playback are intentionally
not adopted. See [integration completion](UPSTREAM-2.8-INTEGRATION.md); the rest
of this assessment is the original planning record.

## Mirror and ancestry

- Previous mirror and last common upstream integration: `4ec1ac829fd2bdf2c571ea215fb3d62477d01220` (SLOOP 2.7.1), an ancestor of MerthSloop.
- Fetched origin and upstream. Both local main and origin/main were exactly two commits behind upstream/main, with zero unique commits; main was not checked out in any registered worktree.
- Fast-forwarded main using compare-and-swap `git update-ref` and pushed normally, without force. Local main, origin/main, upstream/main and independently queried remote origin main now equal `b77bf55e3bf804665b8f5dffb5955bd8443311f0`.
- Active merthsloop remained at `1ce6a8949a241d4111fe2ffa1f260e9ecd8745c6` during review. No checkout, reset, stash, clean, commit, merge, PR, package generation or production edit was performed by this assessment. Concurrent reverb edits and pre-existing untracked demo files were left alone. This report is the only working-tree file written by this task.

## What actually changed upstream

There are two commits, not two independent feature releases:

1. [`beab3f62`](https://github.com/isod89/sloop-fm1/commit/beab3f62d04ff8defcc7b663505bd6bd6ad7836a): SLOOP 2.8 implementation, tests, web updates and release package. Some source comments call intermediate changes 2.7.1 or 2.7.2; they are new relative to our 4ec1ac8 baseline.
2. [`b77bf55e`](https://github.com/isod89/sloop-fm1/commit/b77bf55e3bf804665b8f5dffb5955bd8443311f0): README/SLOOP release documentation only; no additional runtime fixes.

The cumulative diff changes 48 files (2,013 insertions, 413 deletions). Main user features are a NOTES piano roll and note/hit action sheets, more faithful held-note recording, MIDI improvements, filter resonance and punch fade. No new upstream reverb algorithm or chord/arp mode numbering was found in this range.

## Ranked proposal

| Rank | Candidate and benefit | MerthSloop overlap / recommendation | Difficulty |
| --- | --- | --- | --- |
| 1 | USB RESCUE early key scan: drive PA2/backlight low before scanning OCT-. Makes recovery reachable at power-on. Windows loader discovery: accept generic MIDI port names by verified FM-1 handshake, prefer matching pairs, allow 60-second re-enumeration. | Take the HAL/recovery fix and updater logic first. Preserve our installer identity, packaging and recovery instructions. The web change also avoids closing a shared input between probe candidates. Test running firmware, loader-only resume and multiple MIDI devices. | Low–medium |
| 2 | External MIDI clock tempo averaging: last eight beats, 0.8 BPM hysteresis, immediate reset for changes of at least 5 BPM. Reduces displayed tempo and synced delay/arp/slicer jitter while transport still follows pulses. | Take selectively from seq.c; retain our eighth-swing grid, expressive arp timing and sync ownership. Exercise USB/TRS, ramps, START/CONTINUE/STOP and clock loss/reacquisition. | Medium |
| 3 | USB Audio Class collection lists audio streaming before MIDI, with a bcdDevice bump to refresh host descriptor caches. Targets missing audio input on macOS 15 and older. | Take with all descriptor variants reconciled against our current USB build. USB recording remains supported; retain the user's removal of USB playback into the device. Verify Android recording, rate choices, MIDI and reconnect as well as macOS. | Low–medium |
| 4 | Fast encoder turns no longer lose transitions to a two-frame filter. | Take with upstream encoder regression; keep our acceleration preference and UI value scaling. Hardware checks must include slow turns, reverse direction and stationary noise. | Low |
| 5 | Poly recording keeps each overlapping held note until its own release; mono/legato/unison recording marks overlapping notes legato rather than always retriggering them. | High musical value, but adapt to our independent note ties, chord ownership, sequence-fed arp and undo. Do not replace note_ties.c with upstream whole-step editing helpers. Bit-7 legato requires an end-to-end compatibility stage below. | High |
| 6 | CC11 expression scales each track's output without overwriting its mix level; CC121/project load restore full expression. CC0 Bank Select extends Program Change across factory/user sounds; bank 8 addresses user slots directly. | Take after MIDI routing review. Preserve modulation-wheel behavior, MIDI scale mapping, held-note ownership, user-bank numbering and Android sound-bank operations. Include drums in expression and bank-ignore tests. | Medium |
| 7 | Preset-owned arps are replaced with the next sound's arp; an arp the player enabled/edited continues. Fixes every later sound inheriting an unchanged factory arp. ARP page also gains a proper value display. | Adapt deliberately: our browsing preferences, KBD HOLD, latched chords, sequence starters and sequencer-fed ARP must remain predictable. Upstream detects preset ownership by comparing arp values; confirm that policy is sufficient before adopting. Retain appended fork modes and canonical upstream IDs. | Medium–high |
| 8 | Punch fade 0–15 softens FX entry/exit (maximum about 0.37 s); global track-filter resonance 0–10 gives more character; drum FILTER page becomes accessible; noise-drum TUNE changes brightness. | Useful additions, but wait for ongoing spring/new-reverb work in core.h, params.c and fx.c. Resonance is one device setting shared across track filters, not a new per-track parameter or replacement for envelope wah. Preserve locked punch/modulation gestures, sends, saturation/headroom and our effects routing. Take drum tuning with intentional audio-golden changes. | Medium |
| 9 | NOTES piano roll: 8/16/32/64-step zoom, individual chord-note navigation/pitch/delete, force display and OCT+ action sheet for slide/nudge/fill/move/copy/delete. Drum sheet adds ratchet/nudge/fill/move/copy/delete; black keys silently select drum lanes, OCT keys page; optional drum preview and step auto-advance. | Prototype/adapt the visual roll and action sheet in our refreshed UI. We already provide individual note lengths/deletion, move/merge, groove/SHAPE and sequence-starter workflows; upstream move rejects occupied destinations and works on step/tie spans. Keep our richer semantics and controls. Drum key remapping and OCT+ hold conflict with existing gestures and require a conscious interaction design decision. | High |
| 10 | TRACKS shows playing A–D section; SONG section colors follow theme. | Adapt small visual improvements to our HOME layout, selected-kit display, palettes, song library and load/undo feedback. Avoid importing upstream drawing files wholesale. | Low–medium |

## Compatibility and scope decisions

- **Projects:** upstream retains FUN6 and the existing project size. New `SF_LEG = 0x80` records incoming legato on mono/legato/unison NOTE steps; bits 3–6 remain continuation slots. Older firmware ignores legato and reattacks. Our current tie documentation explicitly assumes flags fit seven-bit transport: audit firmware/editor packing, web import/export, Android StepFlags validation/codecs, MIDI-file import, preview, move/delete and normalization before enabling the new flag. A binary file preserving bit 7 does not prove the editor protocol preserves it. Add a canonical upstream 2.8 fixture alongside the existing 2.7.1 fixture, including bit 7 and all four continuation bits.
  Concrete blockers: web/editor.html's STEP_SET builder masks `st.flags & 0x7B`; android/src/Sloop.Sequencing/Validation.cs rejects flags outside `0x7b`, and Models.cs has no legato enum member. A coordinated wire representation is required before claiming editor round trips.
- **Device settings:** upstream packs drum preview and step-advance-off into bits 5/6 of the MIDI drum-channel byte; FX fade into the high nibble of BEND; resonance into bits 2–5 of DR MAP/CLOCK OUT. New readers mask the old values. Preserve upstream prefix semantics plus our MRS1 extension and save-change detection. Test loading older settings, restart persistence, deferred saves and backup/restore; do not blindly restore an old whole-byte BEND/DR MAP decoder. FX fade is a device preference, not a song global to serialize into FUN6.
- **Saving:** upstream does not replace our project_save.c/song-library recovery design. Adapt the handful of project.c settings/expression changes while preserving current save confirmations, durable writes, undo-load and section-switch behavior. Confirm expression reset on all relevant load paths, not only the main project_apply entry.
- **Android:** this upstream range contains no Android changes. Maintain our protocol IDs and capability negotiation, sound banks, FM1 library, scenes and piano-roll semantics. New flags/settings/CC bank behavior need explicit app tests and UI exposure only where useful; do not assume app support from successful firmware compilation.
- **ROM/audio:** upstream reduces SCRCH to its first scratch sample, dropping backspin/rewind to free about 20 KB for the editor, while retaining set IDs. Defer that content cut: it changes how existing sounds play. Measure our raw-sample/cold-table build with all new reverb types first, then choose a budget tradeoff explicitly. Do not import upstream generated WASM, firmware packages, build labels, versions or sample generator changes as incidental merge fallout.
- **Web return-to-stock:** upstream removes the browser stock-return workflow in favor of M-UPGRADE. Treat this as a separate product decision, not a dependency of loader-handshake fixes. Preserve our current installer workflow pending review.

## Suggested integration gates

1. After the shared checkout is free, integrate ranks 1–4 as small changes in an isolated branch; run relevant recovery, encoder, UAC, clock and web discovery tests, then physical recovery/update and host-enumeration checks.
2. Reconcile recording/legato plus MIDI/preset ownership (ranks 5–7), with upstream file round trips, Android/web transport tests and held chord/arp cleanup tests across stop, panic, scene/load and track changes.
3. Once reverb changes settle, add rank 8 with effects render/headroom tests, dual-core equivalence, CPU/flash/RAM budgets and save persistence checks.
4. Review a NOTES/drum-sheet prototype and gesture mapping before ranks 9–10; keep our tie/move/merge and sequence features. Run UI framebuffer and navigation checks plus app regression runners before building a separately approved release.

This task performed source/diff and Git ancestry/ref verification only. No firmware builds, regression runs, conflict simulation or hardware validation were performed; integration difficulty and conflicts above are inferred from inspected code and existing MerthSloop designs. Main is now the clean upstream mirror; MerthSloop integration remains unstarted.
