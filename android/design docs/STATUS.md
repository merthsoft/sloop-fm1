# Current implementation status

This is the current status index. Component docs contain both implemented checkpoints and
future design contracts; dated release/handoff/test results remain historical evidence.

## MIDI file import

Sequence includes MIDI file import: bounded format 0/1 PPQ parsing, source track/channel
selection, destination mapping, audition and undoable app-channel replacement. Physical
FM1 transfer has a separate conversion review at the destination's current DIV/length,
loss counts, converted audition, explicit replacement and fresh-baseline/readback checks.
Other hardware tracks, dormant steps, sound parameters and locks remain intact. No
phone/device validation is claimed for this feature yet. See [MIDI import](MIDI-IMPORT.md).

## Quick sampler and FM1 song library

Sample now has Playable sound and Chopped loop workflows: recording/import, drag
trim handles, pitched audition or 4/8/16 chops and automatic fit/prepare/send with
slot backup and readback. Advanced tools remain available. Live recording, equal
chopping, end-handle trimming and playback initiation were checked on the Pixel;
direct FM1 upload remains hardware validation.

Library now has named FM1 songs, separate from app sessions. Save captures current
project, A–D, arrangement and sound dependencies; Load retains a durable recovery
song before changing hardware and verifies restored objects. Import/export works
offline. Device preferences and MIDI configuration are retained. See
[song-library contract](FM1-SONG-LIBRARY.md); physical hardware round-trip is pending.

Branding is **MerthSloop**, with **MerthSloop Mobile** for Android; firmware
version policy is documented in [BRANDING](../../docs/BRANDING.md). The branch is
`merthsloop`. Independent PR 111 RAM reductions are integrated; dual-core and
low-memory relocation are deferred. Track auto-wah (vocal-range band-pass LFO/signal follower, calibrated for typical synth levels)
and auto-pan are implemented. See [PR assessment](../../docs/firmware/PR-111-ASSESSMENT.md) and
[wah/pan controls](../../docs/firmware/AUTO-WAH-DESIGN.md).

Upstream main/origin main are reconciled through `8fad1a1`: five web commits add
browser firmware audio preview, inline sequence editing, sample gain/level matching,
offline service-worker caching and installation. These coexist with the native
Android app and fork protocols. The browser DSP is rebuilt from our merged sources;
all twelve engines match the native reference sample-for-sample.

## Android chord-mode expansion

Perform has scale-quality Roman numerals, thickness (0–2 extra octave layers),
and 8/15/22-pad octave ranges. Base voice-leading history remains independent of
thickness; 26,880 transition checks and saved-preset compatibility checks pass.
The former Voice lead option is now Smooth voicing; Part writing adds parallel-interval
and tendency-resolution preferences for base chords. The APK builds cleanly and
is installed on the Pixel; larger pad layout and both
selectors were checked live. Simultaneous multi-pad latch, spread/drop voicings,
and additional chord qualities remain proposals, not implemented features.

## Latest firmware

Ordinary keyboard notes support octave handoff: overlap the newly lit key before
releasing the original to sustain without reattack. Held pitches remain fully lit
in their current-octave positions independently of NOTES display.

**MerthSloop 2.6.0.15 is integrated and built locally.** Native per-note tie
extension/selection/shortening/deletion and complete undo preserve overlapping
voices through later attacks. Whole-step ties remain compatible. Independent
linked-voice moves and automatic recording/import of overlaps remain follow-on
work; native move/pitch edits guard linked patterns and whole-track transpose
remains available. Instrument browsing preserves voice/glide/priority/allocation
as well as effects. See [native ties](../../docs/firmware/PER-NOTE-TIES-DESIGN.md).
Native ADD/REPLACE, transient-safe
Note Trails with high-contrast drums and gate-aware held-note strokes, and preserved track effects with both-OCT
reset on normal FX pages are implemented. Upstream audio, inserts, sync,
MIDI, texture, drum mutes and note-length features coexist with the fork libraries,
rhythms, editing, scale lights and performance modulation. Upstream's 27 physical
punch effects replace the previous black-key FX modifiers. The installer is
`http://127.0.0.1:8820/webapp/installer/`. The public GitHub Pages site updates on
push to `merthsloop`; this local development batch is not yet pushed.

Image 581,448 bytes (116 free); static RAM 82,116/98,304 (16,188 free);
pool 332,876/344,064 (11,188 free). Lossless factory
sound sharing, text startup, spare driver-RAM scratch and guarded hot-helper
inlining make the combined build fit. Functional/audio/companion checks pass;
hardware installation/playback remains user testing. See
[release notes](RELEASE-NOTES.md) and the
[integration assessment](../../docs/firmware/UPSTREAM-2.6-INTEGRATION.md).

**2.5 Merthsoft.19** adds independent native starter rhythms on PRESET: ORIGINAL
or 17 explicit choices, shared by Preview/Apply. Installer port 8805. Android
rhythm choice remains future work; existing phone starter requests retain ORIGINAL.

**2.5 Merthsoft.18** replaces held LFO/ENV grids with waveform/cycle displays,
live cents/level readouts and compact key guides. EDIT retains scale-guide labels,
whole-pattern octave and gesture readouts. Installer: port 8804.

Reported device save loss remains under investigation; host overwrite-B/cold-reload
coverage passes and no saving fix is claimed.

**2.5 Merthsoft.16** retains native sequence-browser key, scale, octave, starter,
mode, voice-leading and rhythm choices across track changes until power-off.
The local installer is `http://127.0.0.1:8802/webapp/installer/`.

**2.5 Merthsoft.15** includes the [optimization pass](../../docs/firmware/OPTIMIZATION.md),
retaining all features while saving 816 image bytes and 1,536 static RAM bytes.
The local installer serves this build with its verified package hash.

**2.5 Merthsoft.14** lets HOME lock the held-LFO vibrato or held-ENV tremolo
panel and effect on its captured synth after release. Playing, knobs and scale
lights remain active. HOME/function unlock, STOP/panic, menus and track changes
clear the lock. No new static RAM, protocol or project format. The
[illustrated branch feature guide](../../docs/firmware/MERTHSOFT-FEATURES.md) covers
all delivered native additions with fresh framebuffer captures.

**2.5 Merthsoft.13** adds consistent mapped grid labels and scale-appropriate
accidentals, optional persisted MIDI SCALE/KEYBOARD mapping with held-pitch tracking,
and knob 4 beat sync on vibrato/tremolo. Knob 1 returns to free Hz. MIDI SCALE defaults
OFF so app-generated pitches remain literal. See [scale/MIDI grids](../../docs/firmware/SCALE-MIDI-GRIDS.md).

**2.5 Merthsoft.12** preserves scale-guide and normal keyboard illumination in
held LFO/ENV performance panels. Expanded scale-light tests cover all roots/scales
and actual panel holds/releases, with the guide both enabled and disabled.

**2.5 Merthsoft.11** adds temporary held-LFO vibrato and held-ENV tremolo panels,
affecting only the captured selected synth. Speed/depth/wave/fade are independent
of saved patch parameters; release returns to the previous page. Merthsoft.10 MIDI
CC1 and Android Perform's strip remain available.
See [performance modulation](../../docs/firmware/MODULATION-WHEEL.md).

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

**Historical, superseded by the upstream merge:** 2.5 Merthsoft.3 added black-key punch controls: Slower/Faster, Triplet, Gentle/Extreme,
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

- Physical FM1 remote held/next-bar fills and negotiated sixteen/twenty-seven punch effects, with leased ownership,
  panel priority and stop/disconnect cleanup.
- Persistent FM6 base-voice bank saving with destination review, overwrite confirmation
  and verified readback. Track macros remain separate.
- Negotiated USB return gain/mute and diagnostics.
- Twenty-four native drum groove starters, including four-bar Amen and non-destructive hardware preview, accessible on hardware and from the phone drum
  Sequence workspace. Stopped-only replacement preserves kit/tempo and supports hardware undo.
- Twenty-four native musical sequence starters with root/scale, chord/bass/arp-note modes,
  non-destructive preview, complete replacement undo, native rhythm controls, separate
  generated octave and optional voice leading. Native recording snap and event moves are integrated.
  Musical starters and rhythm shaping are available on hardware and through phone command 76. Independent RHY selection is currently hardware-only; command 74 continues to discover/apply canonical drum grooves.
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
- Firmware resources: audit unused/duplicated runtime buffers and safe mutual-exclusion storage sharing before further sizable additions; the combined capacity audit passes, with only 12 flash bytes free. Lossless factory drum sharing and the text startup screen are now integrated; retain bounded ownership and upstream mergeability.
- Platform/storage: optional local model runtime, Windows/iOS, background services, migrations,
  indexing/collection and durable undo. These are later work, not current launch blockers.

## Verification record

Latest Android solution/APK build passes with zero warnings/errors; all 27 domain runners pass.
Firmware target build and focused host regressions pass with C assertions explicitly enabled.
The Merthsoft.7 APK is installed on the Pixel and cold-launch process survives; new interactive checks wait for an unlocked phone. Earlier cross-session library browsing and SAF draft export/import review/adoption checks passed.
Existing target audio ISR cost budgets still fail and were not relaxed.
See [VERIFICATION.md](VERIFICATION.md) for evidence and limits.

Completed chat assignments and historical evidence are retained in [archive](archive/README.md).
