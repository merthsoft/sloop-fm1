# Android integration checkpoint

Release checkpoint (2026-10-07): the default firmware version is 2.4.2 Merthsoft. The rebuilt
package includes protocol 10, USB playback and SAVE/New; it has not been flashed. All four
parallel slices and the scene/prompt/kit follow-ups are integrated, with 5,041 combined checks plus nine encoding fixtures.
See [PARALLEL-INTEGRATION.md](../planning/PARALLEL-INTEGRATION.md) and [RELEASE-2.4.2.md](../releases/RELEASE-2.4.2.md).

MIDI/octave checkpoint (2026-10-07): Generic MIDI connection mode, per-track generic sequence channel mapping and direct Perform octave/channel controls are integrated. Voice-leading register regressions are fixed. Hardware octave following requires the built protocol-10 firmware; physical buttons and generic instrument output remain acceptance gates. Current shared checks: 61 Workstation and 410 protocol/simulator/sampling.

2026-10-07. The independent component foundations are now connected to the Android
application through `Sloop.Workstation`. The earlier review and its original evidence
remain in [VERIFICATION.md](VERIFICATION.md).

## Available in the app

Integrated scene checkpoint (2026-10-07): typed snapshots, arrangements, musical-boundary
restore and MIDI CC automation are available under Sequence → Scenes & arrangements. Native
pattern/hardware macro/atomic restore remain future work. See [SCENES.md](../../SCENES.md) and
[PARALLEL-SCENES-HANDOFF.md](../handoffs/PARALLEL-SCENES-HANDOFF.md). Hardware atomic scene switching
remains unsupported and is explicitly rejected by the domain API.

**Sound:** edit all six FM6 operators, global voice fields and seven track macros;
choose factory templates; import a DX voice or bank and export a voice; propose bounded
offline prompt refinements or four variations; inspect changes and apply/discard locally.
Locks and undo/redo use the shared sound document. Unsupported negation rejects a request
instead of accidentally producing its opposite. This is a deterministic vocabulary and
recipe system; an offline language-model runtime is not installed.

Physical FM6 read/apply uses protocol 9 GET/PUT, macro reads/writes, fresh comparison of
patch, macros, preset and patch selector, acknowledgments and final readback. Apply changes
the working sound in RAM. Audition plays the current hardware sound: send the local patch
explicitly first. Hardware bank saving and reversible hardware A/B remain future work.

**Sequence:** a local four-bar, 64-position MIDI editor supports note insertion/removal,
pitch, velocity, duration, transpose, quantize, thinning and undo/redo. One-shot MIDI
playback has cancellation, overlapping-note ownership and note-off cleanup on background
or disconnect. It uses synth channels 1–3 and default drum channel 10; configured firmware
channel overrides are not automatically discovered. It is not the final measured realtime
clock, continuous loop, scene or arrangement engine.

Native pattern read/edit/send preserves all four synth slots and all sixteen drum lanes,
including inactive values, ratchets, ties, slides, microtiming, fill and parameter locks.
The adapter requires the checked-in protocol-9 layout, reads all four tracks and restores
the previous selected track. Local native edits can transpose, change levels and quantize
microtiming. Sending compares a fresh full baseline and verifies the complete result.
Multi-command writes are not atomic; interrupted operations require another read and
reconciliation. Changes requiring unsupported wire representations are rejected before writes.

**Sample:** the existing recorder, WAV import and non-destructive trim/chop editor now
feed cancellable worker conversion to mono 22,050 Hz FM1 ADPCM. The editor exposes gain,
downmix, root, chop/drum/instrument mapping, full-slice looping, exact capacity fit,
clipping/cancellation reports, decoded encoded-zone audition and `.fm1` export.
Converted preview uses the same verified Android output routing as WAV preview.

Slot replacement explicitly selects USR1–4 and asks for confirmation. It reads and CRC
checks the existing slot, durably saves its backup and the replacement image and pending
journal, sends aligned chunks, then reads and verifies the resulting slot. Verification
compares header and audio data while allowing the firmware's reserved flash gap to differ.
Pending records survive errors. Library exposes export and backup restoration, including
restoring an originally empty slot. A restore backs up the destination first. This path
is tested against a protocol fixture; physical FM1 flash transfers are not accepted yet.

## Ownership and persistence

The shared native UI now has a compact connection menu, selected track/workspace highlighting,
rounded controls and expandable sections. FM6 groups tone/envelope/keyboard response;
Sequence displays a selected bar as an eight-column grid; Library groups saved files and
recovery images. Sampling separates Chop from Send, supports waveform cursor dragging without
rebuilding, and exposes slot capacity and backup/upload/verification progress. See [UI.md](../../UI.md).

Android depends on Workstation; Workstation coordinates Core, Protocol, SoundDesign,
Sequencing and SampleEncoding. Domain libraries remain independently buildable without
Android. The full solution includes their projects and console suites.

Versioned, bounded local files retain patches/macros, pattern identities/revisions and
the complete native pattern. Candidate documents are saved before becoming live state.
Accepted sound proposals archive prompt, seed, recipes, locks, before/after voice and macro
context, target and parent identity. Draft hashes use explicit deterministic binary encoding
instead of runtime JSON reflection. Unknown/corrupt workspace files are retained separately.
Undo/redo is session-local; persisted content and proposal provenance survive restarts.

Original WAVs remain immutable. Per-asset kit settings and generated artifacts are retained.
Slot backups and transfer journals are flushed before destructive commands. Firmware
readbacks, local drafts and installed hardware state remain distinct; saving locally never
implicitly sends a patch, pattern or kit.

## Validation and remaining acceptance

| Check | Result |
| --- | --- |
| SoundDesign | 1,332 assertions pass |
| Sequencing | 20 scenarios pass |
| SampleEncoding | 102 assertions and nine Python golden fixtures pass |
| Protocol/simulator/sampling | 410 checks pass |
| Workstation | 65 integration checks and 79 kit assignment checks pass |
| Scene hardware | Eight stopped-only fault/reconciliation checks pass; user hardware acceptance recorded in STATUS.md |
| Scenes | 26 catalog/scheduling scenarios pass |
| Android package | Build passes with zero warnings/errors; installed and cold-launched on Pixel 7a |

Pixel checks exercised a bounded current-patch proposal and local apply, note insertion,
library visibility of patch/pattern/proposal files, synthetic PCM16 WAV import, four equal
chops, conversion (22,052/81,408 ADPCM bytes) and encoded-zone playback completion.
The final build also cold-started successfully and restored the inserted sequence note.
Those checks establish app integration, not live FM1 routing or acoustic fidelity.

Workstation checks cover persistence, candidate history isolation, proposal archival,
conversion/mapping/looping, decoded previews, raw FM6 transport, CRC backup/readback,
aligned upload, reserved gaps, native inactive data and lock movement, stale rejection
and backup decoding. A protocol fixture does not emulate the FM1 DSP or USB hardware.

USB playback firmware passes focused functional host tests and target linking/RAM/flash
checks. The shared runner now includes those focused tests and static budget diagnostics
include the playback mixer. Existing static CPU budgets still fail for both baseline and
playback builds; budgets were not relaxed. FM1 enumeration, sustained duplex audio,
worst-case DSP deadlines, latency, fidelity and live patch/pattern/sample transfer tests
are historical hardware checklist items; the user accepts hardware testing for now. No
firmware was flashed by this chat during integration.

The chord/performance instrument is now implemented; see [PERFORMANCE.md](../../PERFORMANCE.md).
Recording/presets, algorithm diagrams, app scenes/automation and full session adoption are
implemented. Remaining work includes piano roll/native inspection, external clock, sampling
pitch/per-chop tools, configurable performance macros, hardware A/B/bank saving, atomic live
hardware scenes, offline loop composition and optional local inference. See NEXT-WAVE.md.
Current firmware remains three synth tracks plus one drum track.


