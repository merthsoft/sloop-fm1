# Current implementation status — October 8, 2026

This is the current status index. Component docs contain both implemented checkpoints and
future design contracts; dated release/handoff/test results remain historical evidence.

## Hardware acceptance

The user considers hands-on hardware validation complete for now (October 8, 2026).
It is not an outstanding gate for continued implementation. This records user acceptance,
not new agent measurements or proof that every historical stress checklist was executed.
Earlier statements that physical acceptance is pending describe the earlier checkpoints.
Revisit hardware only for a new failure or a material feature change; do not repeat the old gate.

## Implemented and integrated

- Android C# shell, five workspaces, SLOOP connection, generic MIDI output and limited simulator.
- Full manual FM6 editor, algorithm diagrams, envelope dragging, operator copy/swap, numeric
  entry, SysEx exchange, local history, bounded offline sound recipes and prompt edit proposals.
- Reversible hardware FM6 A/B audition with guarded readback, process-retained originals,
  explicit RAM-only Keep/Restore, and recovery after workspace replacement/disconnect.
- App/native pattern editing, reviewed prompt edits, continuous MIDI looping and Perform capture.
- Touch app piano roll with select/draw/move/resize/pan/quantize, shared note protections and
  undo, multi-note selection and group move/resize/delete/quantize. Offline deterministic
  composition supports reviewed/editable loops, part regeneration, MIDI draft audition,
  saved local drafts, tempo, progression and rhythm controls.
- Eight chord pads including I ↑, quality joystick, scale grid, keyboard, drum pads, ribbon,
  block/strum/arp/repeat, voice leading, latch and saved performance presets.
- Direct key/scale/octave controls. SLOOP routes by selected synth; generic mode supports
  channel override. Pad velocity is steady. Octave changes preserve latched chord pitches
  until the next chord press; I→vii→I preserves register. Earlier APK installed on Pixel 7a.
- WAV import, microphone/USB capture, trim/chop/undo, zoom/pan, transient proposals, exact
  edges, source/encoded audition, mapping/fit/ADPCM conversion, backed-up upload/readback/restore.
- Named sessions, portable validated archives, complete staged session adoption and recovery.
- Pitch/root estimates and manual override, per-chop gain/tuning, processed preview and matching
  kit conversion. Chop settings travel with sessions and are validated before adoption.
- Retained sample browsing/reuse within the active session and playback-position tap chopping
  with review, marker undo, optional latency compensation and one-step Apply undo.
- Custom chord/joystick mappings and single-owner XY MIDI CC macro with release defaults;
  mapping/macro settings travel with performance presets and sessions.
- App scenes, management, beat/bar/phrase switching, repeating/reordered arrangements and
  stepped MIDI CC automation. Explicit stopped-only FM1 scene sound transfer is integrated.
- Whole-project load undo/redo in firmware 2.4.16: stopped EDIT + OCT− restores the
  project preceding the latest saved-slot load, stopped section selection or working-project
  backup restore; OCT+ redoes it. Subsequent sequence edits/recording supersede load history.
  Snapshot is volatile and cannot recover loads performed on older firmware.
- FM1 USB playback path. Firmware 2.4.16 Merthsoft retains SCL 2 knob 4 LATCH; non-CHROM
  modifiers toggle on latched chords until pressed again. CHROM gives literal black-key roots. Firmware package and local
  installer are built. This chat has not flashed it.

- Physical FM1 remote held/next-bar fills and sixteen punch effects, with leased ownership,
  panel priority and stop/disconnect cleanup.
- Persistent FM6 base-voice bank saving with destination review, overwrite confirmation
  and verified readback. Track macros remain separate.
- Negotiated USB return gain/mute and diagnostics.
- Sixteen native drum groove starters, including four-bar Amen and non-destructive hardware preview, accessible on hardware and from the phone drum
  Sequence workspace. Stopped-only replacement preserves kit/tempo and supports hardware undo.
- Opt-in sequencer-fed arpeggios through ARP 2 ORD SNOTE/SPLAY, with independent
  sequence/live membership, NOTE/TIE/REST lifecycle and balanced generated output.
  General local/USB/TRS owner aggregation and semantic harmony/followers remain future work.
- Searchable sample library across saved sessions, staged byte-exact reuse with associated
  chop/audio/kit settings and cancellation; current originals and source snapshots remain intact.
- Portable validated composition draft export/import, exact edited notes and generator identity,
  explicit review before publication and stale workspace/pattern guards.

## Remaining product work

- Sampling: combined USR3+4, broader format import and time stretching.
- Sequencing: richer native inspection, external clock and timing telemetry.
- Harmony: full live source ownership, independent source/output routing and semantic chord followers.
- Composition: more flexible progressions/rhythms and free-form
  interpretation. Drafts currently save on this phone outside session archives; no local language model is included.
- Perform: external controller input and CC gesture recording.
- Sound: proven-device reconnect recovery, broader engine editing and local rendering.
- Hardware scenes: atomic boundary application of hardware sound/pattern/tempo; current
  stopped-only sound transfer does not provide that operation or upload scene samples.
  The protocol/staging scaffold is tested but disabled in production until complete engine hooks exist.
- Platform/storage: optional local model runtime, Windows/iOS, background services, migrations,
  indexing/collection and durable undo. These are later work, not current launch blockers.

## Verification record

Latest Android solution/APK build passes with zero warnings/errors; all 25 domain runners pass.
Firmware target build and focused host regressions pass with C assertions explicitly enabled.
The updated APK is installed on the Pixel; cold launch, cross-session library browsing and SAF draft export/import review/adoption pass.
Existing target audio ISR cost budgets still fail and were not relaxed.
See [VERIFICATION.md](VERIFICATION.md) for evidence and limits.

Completed chat assignments and historical evidence are retained in [archive](archive/README.md).
