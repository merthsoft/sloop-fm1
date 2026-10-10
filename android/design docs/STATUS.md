# Current implementation status — October 9, 2026

This is the current status index. Component docs contain both implemented checkpoints and
future design contracts; dated release/handoff/test results remain historical evidence.

## Latest firmware

**2.5 Merthsoft.10** adds transient vibrato through held LFO + knob 1, MIDI CC1 and
an Android Perform touch strip. Physical priority, release/STOP/panic/reset cleanup
and USB/TRS source cleanup preserve saved patch values and held-note ownership.
See [mod-wheel contract](../../docs/firmware/MODULATION-WHEEL.md).


**2.5 Merthsoft.9** aligns held SEL and scale LEDs with the selected synth’s SEL-page
ROOT/SCALE and suppresses generic keyboard backlight while the guide is active.
Held key/scale edits retain their all-synth broadcast; tapped-page edits stay per-track.


**2.5 Merthsoft.8** adds persistent keyboard scale lights: hold physical SEL and
press OCT+ to toggle a dim guide using song ROOT/SCALE, leaving pressed notes bright.
It does not alter octave, latch or sound; drum/grid/control layers retain their LEDs.
The preference uses existing device settings, with writes deferred during playback.
See [scale-light design](../../docs/firmware/KEYBOARD-SCALE-LIGHTS.md).


**2.5 Merthsoft.7** integrates phone browsing, audition and confirmed application of the native musical bank with key/scale/octave/mode/VLEAD and rhythm shaping. Command 76 requires protocol 14 and capability discovery; its lease expires on disconnect, takeover or missing renewal. Arpeggios preserve source velocity across audio, MIDI and recording, with strongest live/sequence shared-pitch expression. Android Perform now offers opt-in external controller device/port input, literal pitches or bounded scale-root chords, sustain and safe lifecycle cleanup. On native STEP, held OCT + knob 2 edits whole octaves and knob 3 moves events, including later onsets inside their own tie runs. Turning the SELECT knob between starter subpages preserves preview.

**2.5 Merthsoft.6** adds STEP-page tie/rest painting: select the starting note,
hold OCT+ (ties) or OCT− (rests), and turn STEP clockwise. Each crossed step is
written without changing the starting step; the cursor stops at the pattern end.
Backward movement preserves existing steps. One hold is one undo session; ordinary
cursor wrapping and OCT controls outside STEP remain unchanged.
The original Merthsoft.6 knob-2 move gesture (now knob 3) moves a complete note/tie chain while stopped,
including timing, conditions and locks, with collision protection and metadata undo.
RECORD-page SNAP chooses TRACK/1/8/1/4 independently of playback DIV; it is a per-track
preference until reboot. Incompatible/finer grids show DIV LIMIT and use track snapping.
The musical bank has 24 entries, starts CHR tracks in Major without changing track scale,
and has a third pitch page for independent octave and optional voice leading. The shared
nearest-inversion helper serves live chords and deterministic starter rendering.

**2.5 Merthsoft.5** distinguishes ARP NOTES from BASS across every musical starter:
ARP adds eighth-note attacks while retaining syncopation and cycles all triad/seventh
tones by attack order; BASS keeps lower roots and the original rhythm.

**2.5 Merthsoft.4** adds twelve native musical sequence starters, expands the drum bank
to 24 grooves, and adds rhythm shaping and non-destructive audition. Native OCT+ holds
confirm replacement after 700 ms. Both libraries use the same complete pattern/metadata
undo supplement. Synth SEQ navigation reaches STEP, PATTERN, SEQUENCES and SONG through
physical taps and SELECT; its browser includes a rhythm subpage. Drum pages are GRID,
KIT, GROOVE and SHAPE. SNOTE route changes preserve a currently sustained chord when
turning ARP off/on, including per-note dynamics. Lossless one-bit font packing saves
16,128 bytes of bitmap data while preserving pixels and legacy header compatibility.
See [sequence starters](../../docs/firmware/SEQUENCE-STARTERS-DESIGN.md),
[drum grooves](../../docs/firmware/DRUM-GROOVES-DESIGN.md) and
[rhythm shaping](../../docs/firmware/RHYTHM-SHAPING-DESIGN.md).

**2.5 Merthsoft.3** adds black-key punch controls: Slower/Faster, Triplet, Gentle/Extreme,
Blend, Latch and Retrigger. Momentary controls clear on FX exit; latch survives key/layer
release and clears on toggle or STOP. Controls retain physical/remote FX priority and do
not emit or record synth notes. FX tiles show mappings. See [component design](../../docs/firmware/PUNCH-FX-DESIGN.md).


**2.5 Merthsoft.2** extends the 700 ms ARP/SEL hold gesture to manually played arpeggios
with CHORD OFF. Keys stay physically held during capture; releasing them preserves the
arp pool. A second button hold toggles latch off without cutting physically held notes.
Generated chord and modifier latch behavior remains covered by real UI/audio tests.

## Upstream 2.5 integration

Release **2.5 Merthsoft.1** incorporates upstream `fa9ce57` while retaining the Android
workstation, grooves/preview, chord latch/modifiers, sequencer arps and project-load undo.
PHYS/NOISE engines, shared engine memory, SYN drum kits and 48 kHz USB capture are integrated.
USB playback remains 44.1 kHz and negotiates independently of capture. INFO protocol 13
keeps companion commands 72–75; SYN commands are 80–84. Web routing also supports upstream
protocol 10. Fourteen visualizers remain; removed persisted styles fall back safely.
Original Android work is Unlicensed with GPL/Apache exceptions: [licensing](../LICENSING.md).
Releases increment `Merthsoft.N`, keeping the upstream base version visible. Development
uses branch `android`; local `main` tracks the upstream base.

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
- Twenty-four native drum groove starters, including four-bar Amen and non-destructive hardware preview, accessible on hardware and from the phone drum
  Sequence workspace. Stopped-only replacement preserves kit/tempo and supports hardware undo.
- Twenty-four native musical sequence starters with root/scale, chord/bass/arp-note modes,
  non-destructive preview, complete replacement undo, native rhythm controls, separate
  generated octave and optional voice leading. Native recording snap and event moves are integrated.
  Musical starters and rhythm controls are available on hardware and through phone command 76; command 74 continues to discover/apply canonical drum grooves.
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
- Perform: CC gesture recording and broader external-input expressive modes.
- Sound: proven-device reconnect recovery, broader engine editing and local rendering.
- Hardware scenes: atomic boundary application of hardware sound/pattern/tempo; current
  stopped-only sound transfer does not provide that operation or upload scene samples.
  The protocol/staging scaffold is tested but disabled in production until complete engine hooks exist.
- Firmware resources: audit unused/duplicated runtime buffers and safe mutual-exclusion storage sharing before further sizable additions; current static headroom is 892 bytes. Keep changes scoped for upstream merges.
- Platform/storage: optional local model runtime, Windows/iOS, background services, migrations,
  indexing/collection and durable undo. These are later work, not current launch blockers.

## Verification record

Latest Android solution/APK build passes with zero warnings/errors; all 27 domain runners pass.
Firmware target build and focused host regressions pass with C assertions explicitly enabled.
The Merthsoft.7 APK is installed on the Pixel and cold-launch process survives; new interactive checks wait for an unlocked phone. Earlier cross-session library browsing and SAF draft export/import review/adoption checks passed.
Existing target audio ISR cost budgets still fail and were not relaxed.
See [VERIFICATION.md](VERIFICATION.md) for evidence and limits.

Completed chat assignments and historical evidence are retained in [archive](archive/README.md).
