# MerthSloop firmware 2.7.1.34

- HOME shows the selected drum kit name instead of the generic DRUM label.
  Kit changes refresh the label immediately, including while stopped.
- Renderer regression covers every kit and compares incremental/full redraw.

# MerthSloop firmware 2.7.1.33

- VINYL's maximum settings are now audibly stronger: slow wow/flutter, darker
  TONE and louder crackle, with exact zero bypass and smooth control transitions.
- VINYL replaces MONO as a held punch-in; releasing restores persistent settings.
  DOUBLE TIME replaces octave down and complements HALF without changing pitch
  or stored BPM. External sync bypasses the two rate gestures.
- SPRING REVERB knob 4 TIME adjusts reflection spacing independently from SIZE's
  nominal decay target. Uses the existing buffer; FREEZE preserves its length.
  Model/time metadata does not overwrite the drum-tuning checksum; default
  projects retain canonical upstream byte identity.
- Image 581,176 bytes, 388 free; main static RAM 85,656, low RAM/pool unchanged.
  Audibility, stereo/release, full-mixer, timing, format and UI regressions pass.
  Hardware listening and deadline measurements for this build remain pending.

# MerthSloop firmware 2.7.1.30

- Explicit section SAVE also updates the existing power-on working-loop object,
  preventing an older root/scale from returning during background autosave's
  20-second rate limit. The named section still holds the snapshot taken at SAVE;
  a queued flush captures the then-current working loop for startup.
- Queued section writes begin once stopped and voices are quiet, without the
  routine settings input-idle delay. Failed startup writes keep the save pending
  and cannot produce a successful-save confirmation.
- Adds C# minor → G Phrygian cold-NOR/restart regressions for stopped/queued saves,
  plus startup-write failure/retry checks. Assertions explicitly remain enabled
  under optimized host compilers. The old behavior fails the new restart test.
- Upstream FUN6 compatibility and project-format tests pass; storage object IDs
  and serialized layout are unchanged. Image 575,160 bytes, 6,404 free, no RAM growth.

# MerthSloop firmware 2.7.1.29

- Polishes settings tabs, long custom-palette names/swatches, About credits and
  complete engine summary values/units. Adds a gutter between header icon and
  track number; About shows the version followed by MERTHSOFT.
- Keeps destination bars within their guide at parameter extrema; abbreviates
  the held-step ratchet card deliberately rather than clipping its word.
- Replaces OCT UP with live OCT DOWN on the same punch-in key. Three-pitch DSP
  tests verify octave-down output and exact dry recovery; all 27 effects pass.
  Transport HALF and simultaneous effects are separate design work.
- Image 575,016 bytes, 6,548 free; RAM, low RAM and pool unchanged. Complete-glyph,
  sanitized UI/fuzz, audio golden, static-analysis and target checks pass.
  No Android APK change or hardware timing claim is included.

# MerthSloop firmware 2.7.1.28

- Withdraws provisional FM6 diagrams; engine pages return to parameter summaries.
- SONG uses shared knob cards, palette-aware section colors, standard header and
  footer. Record-ready/count-in align with the same layout; free takes retain
  their large timer. Disabled controls show muted dashes.
- Consolidates the card primitive/strip, removing the legacy bottom-dial renderer
  and duplicate record header. Help and footer share engine-page titles.
- Generic destination/filter/delay guides start higher, leaving clear space
  between the DLY caption and footer.
- Image 574,856 bytes, 6,708 free; RAM 81,592, low RAM/pool unchanged. Consistency
  changes recover 1,944 image bytes; recording/song interaction and sanitized
  redraw/fuzz tests pass. Physical timing has not been measured.

# MerthSloop firmware 2.7.1.27

- Removes unused right-channel scope capture and volatile note/kick animation
  telemetry from retired visualizers: 1,040 static RAM bytes and 32 image bytes
  recovered. The seven retained captures and all 182 audio goldens are unchanged.
- Fixes a flash-only size assertion in the RAM-only firmware configuration.
- Moves the DLY guide up and reserves space for its complete bottom caption.
- Adds a reproducible target-compiler static-analysis tool and detailed audit.
- Image 576,800 bytes, 4,764 free; static RAM 81,592, low RAM/pool unchanged.
  Target budgets/inlining and sanitized UI regressions pass. Hardware timing
  was not measured. No new Android APK is included.

# MerthSloop firmware 2.7.1.26

- Held SAVE section stores write immediately when stopped. During playback they
  say QUEUED: STOP TO SAVE, then confirm after a successful deferred flash write.
- Explicit section/project saves commit device settings, including 1/8 swing
  mode. Failed section/settings writes remain retryable rather than claiming a
  successful durable save.
- Real NOR-storage regression covers track/drum levels, global swing, cold reload,
  queued snapshot isolation and write failures. Fixes the combined wah/pan LED
  lookup found by the full UI sanitizer regression.
- Image 576,832 bytes, 4,732 free; static RAM 82,632; pool unchanged.

# MerthSloop firmware 2.7.1.25

- Keeps exactly seven visualizers: Oscilloscope, Spectrum, Spectrogram, VU Meters,
  Note Trails, Groove and Song Journey. Selector wraps 1/7 through 7/7.
- Retained saved IDs remain stable; removed IDs fall back to Oscilloscope.
- Removes unused renderers and animation state: 2,912 image bytes and 48 static
  RAM bytes reclaimed. All seven render and settings/navigation regressions pass.

# MerthSloop firmware 2.7.1.24

- HOME's striped sun pulses gently with the transport beat while playing and
  returns to its original brightness when stopped. No extra persistent RAM.
- Beat-phase invalidation works with stationary playheads/long notes; tests
  confirm track-lane/footer pixels remain unchanged by the sun animation.
- Equivalent compact card borders and shorter hints offset most new code:
  579,512 image bytes, 2,052 bytes free; four image bytes above the preceding
  engine/help checkpoint. Target DSP budgets/inlining remain unchanged.
- UBSan renderer and 20,000-frame UI regression pass. Physical redraw cost has
  not been measured. No Android APK update is included.

# MerthSloop firmware 2.7.1.23

- FM6 OPS/PATCH show the active algorithm: six operators, modulation routes,
  carriers and feedback. Patch and algorithm edits invalidate the diagram.
- HOME includes the engine name beside the preset in the selected track color.
- Help puts PRESETS left and ALGORITHM right; hold hints name the actual action,
  including LFO vibrato and ENV tremolo.
- Image 579,508 bytes; flash free 2,056. No static-RAM or audio-pool growth.
  ANALOG engine diagrams are deferred to retain the flash reserve.
- All 32 routing diagrams, dirty/full redraws and patch isolation pass under
  UBSan; the 20,000-frame UI suite and 182 unchanged audio goldens pass.
- Firmware-only update; no new Android APK or hardware timing measurement claimed.

# MerthSloop firmware 2.7.1.22

- Refines HOME's NEON scene with a larger round striped sunset, a warm-to-violet
  gradient and wireframe mountains. Track lanes remain readable underneath.
- Adds 192 image bytes, no additional RAM. Image 578,324 bytes; flash free 3,240.
- Native redraw/palette/state tests pass; physical display timing remains pending.

# MerthSloop firmware 2.7.1.21

- Native UI overhaul: shared knob cards, NEON colors and four-track overview.
- Refreshed held maps, drum/groove and progression browsing, and track-local
  modulation displays; existing gestures and musical behavior are retained.
- Parameter guides and SCREEN → PAGE HELP; existing palettes/custom themes remain.
- 578,132 image bytes, 3,432 flash bytes free, main static RAM 82,680; no additional
  framebuffer or audio-pool allocation. The raw factory sample path remains.
- Real-renderer screen/video captures and host regressions pass. Hardware UI
  latency and LCD contention are pending, not represented as tested.
- No new Android APK is claimed for this firmware-only UI refresh.

# MerthSloop 2.7.1.20

- Restores direct factory sample reads, removing render-time decompression/caches.
- Expands compact lookup tables only at cold startup, before audio starts.
- Removes phone/computer USB audio playback into the FM-1, return controls and
  phone access to musical sequence starters. USB recording/MIDI, native sequences
  and phone access to drum grooves remain.
- Firmware: 578,664 image bytes, 2,900 flash bytes free; main RAM 82,776;
  low RAM 22,772; pool 332,876. Existing project and sample formats are unchanged.

Earlier release checkpoints below retain their original historical evidence.

---

# Latest firmware — MerthSloop 2.7.1.19

- Integrates upstream 2.7.1 with canonical song flags, arp IDs and settings. No
  migration of old-fork formats; upstream-produced FUN6 fixture roundtrips exactly.
- Dual-core safe fallback, drum tune/decay, held sample loops/tails, QUANT OFF and
  recording over held notes, arp modes/KBD HOLD, favorites/themes/acceleration,
  MIDI3 remote/clock and web LIVE/song features join existing fork workflows.
- INFO protocol 15: upstream SYN 72–76, LIVE 77; companion extensions 85–89. Android validation
  follows upstream continuations/held-loop metadata and guards capability discovery.
- Fixes HOLD ownership vs chord latch, duplicate timing undo, SELECT navigation
  through custom starter pages, and persistence of side-record-only preferences.
- Lossless factory sample packing saves 33,744 bytes net. Image 559,992 bytes,
  static RAM 74,888, low RAM 22,772, pool 333,308. All 182 renders match the raw build.
- Host/source checks pass as recorded in the integration guide. Physical hardware
  checks remain pending. The updated ARM64 APK is built.

## Previous checkpoint

# Latest firmware — MerthSloop 2.6.0.18

- HOME menu → TIMING → SWING GRID selects STEP (existing behavior) or 1/8.
  TRACKS shows SWING8 in eighth mode. HALF TIME hats and eighth-note arps can
  shuffle on a 1/16 pattern. The setting survives power-off without a project change.
- Preview, recording snap, straight SLICER rates and GROOVE markers follow the grid.
  Eighth-mode nudged events/gates/ratchets use their own step width. Grid lines no
  longer overwrite shifted markers. Live ARP retains its own ARP 2 swing.
- Compact aligned ROM groove masks retain every factory hit. Full image 581,360
  bytes (204 free); static RAM 82,116 and pool 332,876 bytes are unchanged.
- Actual mixer tests check HALF TIME at 0/375/500/875ms, legacy compatibility,
  uneven loop and early/late FEEL preview parity, settings and visualizer pixels.
- Local/origin main mirror upstream 2.7.1; feature-branch integration remains
  separate. The assessment records flag, arp-ID, settings and memory conflicts.

## Previous integrated checkpoint — MerthSloop 2.6.0.17

- Native CUSTOM progressions: count 1–8, editable degree per position and 4/8/16-step
  span within the 64-step pattern. Shared rhythm/key/voicing/mode controls, preview,
  ADD/REPLACE and undo. Applied notes save with the song; draft lasts until power-off.
- Shared sequence rendering and compact ROM degrees fit the new page. A 100,000-event
  comparison preserves factory starter output. Custom count/span/rhythm/mode,
  preview, undo and navigation tests pass, alongside the existing UI fuzz and ties.
- Swing scheduling and visualizer regressions verify 0/50/100 on odd sequencer
  steps. No swing behavior has been changed pending the reported setup details.
- Target 581,560 bytes (4 free); static RAM 82,116 and pool 332,876 bytes.

## Previous integrated checkpoint — MerthSloop 2.6.0.16

- STEP renders all tied chord voices, including at bank boundaries. Independent
  continuations connect without gaps; stale rest/tie payload cannot distort range.
- Redraw caching tracks all four pitches. STEP knob zoom no longer covers notes.
- NOTE and LEN share the anchored source (SRC/SEL). Shortening past the cursor
  no longer makes subsequent LEN edits target an empty destination step.
- Added pixel-level regression checks and anchored-length control tests. Playback,
  editing, move/merge and the 20,000-frame UI fuzz pass. Hardware audition pending.
- Image 581,512 bytes (52 free); static RAM and pool unchanged from the tie release.

## Previous integrated checkpoint — MerthSloop 2.6.0.15

- Independent note sustains can cross later note onsets. Hold OCT+ and scrub STEP
  with knob 1; knob 4 chooses ALL or one source pitch. LEN can shorten that
  selection, and EDIT deletes it without erasing the other voices. Use POLY.
- Legacy ties and saved project layout remain compatible. Single-event move and
  transpose are guarded for independently tied patterns; whole-pattern transpose
  remains available. Live recording and phone/web duration editing are follow-on work.
- Instrument browsing preserves voice mode (including LEGATO), glide, priority and
  allocation alongside track effects. Explicit sound initialization retains defaults.
- Lossless shared icon rows and measured UI code reductions fit the additions:
  target image 581,448 bytes (116 free), static RAM 82,116 bytes. No audio-path
  icon decoding or new pattern buffers.
- Native playback/editing, voice identity, MIDI balance, project round-trip,
  icon pixel equivalence and browser/native audio regressions pass.

## Previous integrated checkpoint — MerthSloop 2.6.0.14

- Wah uses a dedicated, narrower resonant band-pass response: it emphasizes a moving
  vocal-range band and rejects lows/highs instead of only sweeping brightness.
- Knob 3 is CENTER; the upper sweep stays in the useful wah range. Makeup gain is
  controlled. LFO and envelope modes use the new response; ordinary filters are unchanged.
- Real DSP tests check low/high rejection, movement of the resonant peak, output
  bounds and ordinary-filter golden compatibility. No new runtime buffers.
- About retains name, version and attribution; its decorative sail is removed to
  fit the instrument feature. Target image 581,512 bytes (52 free).

## Previous integrated checkpoint — MerthSloop 2.6.0.13

- Auto-pan uses LFO+FX; ENV+FX remains auto-wah.
- Ordinary keyboard notes support overlap handoff after an octave switch without
  a new attack. The note ends only after its last physical key releases.
- Held pitches fully light their mapped key in the current octave even when NOTES
  display is disabled. Other tracks and generated chord/arp behavior remain independent.
- Tests cover both octave directions, both release orders, balanced MIDI output,
  unchanged voice age, final release and a deliberate fresh attack.
- Target image 581,548 bytes; static RAM and pool unchanged.

## Previous integrated checkpoint — MerthSloop 2.6.0.12

- Envelope wah responds to typical synth levels: sensitivity gain is corrected and
  defaults favor a pronounced attack/release sweep.
- Wah now uses a resonant low-pass with a useful dark-to-bright cutoff range,
  retaining resonance at the open end. Ordinary track/global filters are unchanged.
- DSP response tests verify frequency sweep, resonant emphasis and sensitivity;
  all 182 inactive-effect golden renders remain unchanged.
- Target image 581,276 bytes (288 free); static RAM and pool unchanged.

## Previous integrated checkpoint — MerthSloop 2.6.0.11

- ENV+FX: selected-synth auto-wah, free/beat-synced LFO or own-signal envelope follower.
- SEL+FX: selected-synth auto-pan, waveform/depth and beat sync.
- Both preserve keyboard/scale lights, support HOME locking and clear on release,
  STOP/panic or track change. They do not overwrite saved patch parameters.
- PR 111 independent memory adaptations: packed MIDI owners, DRAWBAR arena state,
  shared main-loop/OTA scratch and flash XIP preset banks. Dual-core is deferred.
- STEP OCT+/OCT− help persists while held, with transient warning priority.
- MerthSloop startup, About, installer/editor and Android labels. Firmware uses
  upstream major.minor.revision plus independent build number, now 2.6.0.11.
- Target: image 581,420 bytes, static RAM 82,100 bytes, pool 332,876 bytes.

## Previous integrated checkpoint — 2.6 Merthsoft.10

- Hold LFO or ENV and tap OCT+ to apply vibrato/tremolo parameters to the track's
  saved shared LFO. Rate, waveform and beat division transfer; tremolo depth is
  exact and vibrato depth rounds to the regular pitch destination's steps. The
  temporary effect stops to avoid doubling; unrelated destination depths stay.
- LOFI MOTN ARP retains OFF/OCT/MAJ/MIN and adds every SEL chord shape, using the
  existing harmony builder and root/scale. No additional voices or persistent RAM.
- Identical font bitmaps share offsets: 816 bytes less data, unchanged renderer
  and exact pixels for every supported character.

- STEP move brush now merges a moved onset into an existing chord (four-note
  limit, no duplicate pitches). Destination timing/dynamics/locks remain; a single
  source note takes its duration, or tied events require matching lengths. Source
  locks and overflow reject before editing. Complete undo/redo is retained.
- Duplicate LOFI MAJOR/MINOR menu entries are removed; MAJ/MIN already cover them.
  Exact suffix/prefix glyph sharing saves a further 608 bytes, with no renderer change.

- Note Trails excludes released-voice/delay-tail continuation and cuts the current
  synth/drum column during the fully wet closed master GATE phase. Partial wet
  retains the dry path; pending closed-phase hits are not replayed on reopening.

- GLO SYSTEM always shows CPU percentage, including standalone use without USB.
  The header still indicates a configured USB connection.
- Note Trails sustained notes now draw continuously across time columns; overlap
  color stripes remain visible. The earlier narrow strokes caused artificial gaps.
- ADD de-duplicates existing pitches as well as incoming pitches and preserves
  tie slide flags. Both Apply gestures, undo/redo and 1,000 independent randomized
  pitch-set cases pass; late overflow is atomic.
- Native SEQUENCES: ALGORITHM selects ADD/REPLACE. ADD retains current timing,
  locks and fill, merges tied notes, de-duplicates pitches, and rejects fifth-note
  overlaps atomically. Preview remains the starter alone; hardware undo/redo works.
- Note Trails captures short attacks across both UI frames, distinguishes coincident
  track pitches, keeps the cursor clear of notes, and uses white-on-black drum hits.
- Instrument changes preserve track effects across engine/preset/user-preset browsing
  and MIDI Program Change. Both OCT buttons on a normal FX page reset that track's
  effects. INIT SOUND and song loading retain their intentional reset/load behavior.
- Factory base records are bit-packed losslessly: 968 bytes less data, 64 extra
  decoder bytes and a 21-byte table; the measured net target saving is 880 bytes.

Upstream SLOOP 2.6 is integrated into merthsloop. Local main and origin/main
match upstream revision 8fad1a1. Five follow-up web commits add firmware WASM audio
preview, inline sequence editing, sample gain/level matching and offline/PWA support.
The browser adapter uses the fork's multi-note arp cleanup and firmware memory routines;
all twelve engines match the native reference sample-for-sample.

- Upstream's 27 punch effects replace the former black-key FX modifiers. Remote
  performance negotiates sixteen or twenty-seven effects with the connected firmware.
- Track inserts, synced envelopes/LFOs, filter modes, texture, drum mutes,
  note length/velocity, MIDI sustain/bend/routing/learn and step entry are integrated.
- Native groove/progression libraries, independent starter rhythms, sequenced arps,
  physical latch gestures, scale guides/lights, editing tools, held/locked modulation,
  complete song-load undo and the Android/editor protocols are retained.
- FUN6 imports older projects and shifts engine locks correctly. Web backups include
  MIDI object 10. Android negotiates 61/53 or 71/63 parameter layouts, configured
  channels including OFF, and correctly routes optional bass.
- Factory drum definitions are losslessly shared; all 512 expanded sounds match.
  Startup uses a text title instead of the bitmap logo. Size compilation preserves
  hot DSP inlining, guarded against newly outlined per-sample helper calls.

Target image **581,060 bytes** (504 free); static RAM **94,500/98,304**
(3,804 free); pool **333,900/344,064** (10,164 free). Fully overwritten
scratch uses 8,800 bytes beside the flash driver, leaving 12,820 bytes there;
linker/build checks preserve an 8 KiB driver reserve. Stack guards are unchanged.
HAL and call-free RAM driver checks pass.

Package **609,941 bytes**, identity **FM-1_900**. SHA-256:
`5120a513bf70ad40e0b881983f3346367fa766d5334948258922b1067e11c0cf`.
Installer: http://127.0.0.1:8815/webapp/installer/

Host functional, serialization, UI, DSP and divide-by-zero checks pass. All 182
reviewed audio renders match, with zero health, voice/routing or crash failures.
The one LOFI arp fixture change preserves source velocity; forcing velocity 100
alone reproduces the exact upstream fixture. Android builds with zero warnings/errors;
web and companion domain tests pass. Hardware flashing/playback was not performed
in this integration session. Static instruction/call checks are not cycle measurements.

Unmarked settings words are interpreted as old fork settings. Upstream's old
channel-filter bits overlap fork visualizer/scale flags, so imported upstream
settings may need channel-filter re-selection. New settings have a format marker.
MIDI configuration is device-wide, not saved independently with each song.

The earlier intermittent song-save report remains unresolved. NOR round-trip tests
pass; no claim is made that its hardware cause has been fixed. A pre-existing
settings-write failure also lacks automatic retry; it remains follow-up work.

## Android chord labels

Quick sampler workflows and the named native FM1 song library are integrated.
Song loading retains a recovery archive before mutation, restores shared sound
dependencies and arrangement, then verifies objects and edited FM6 track voices.
Physical FM1 round-trip remains pending; see [song library](FM1-SONG-LIBRARY.md).

- Perform chord pads and mapping choices derive Roman numeral case and diminished/augmented
  marks from the selected scale, matching the default diatonic chords they play.
- Workstation tests cover all eight degrees in major, natural/harmonic minor, both
  pentatonic parent harmonies, and the differing Dorian/Mixolydian chord qualities.

## Android expanded chord mode

- Thickness adds one or two upper octave copies after base voice leading, with
  duplicate/out-of-range notes omitted. Optional bass remains unchanged.
- Pad range offers 8/15/22 pads for direct access to higher octave degrees. Higher
  pads retain explicit register and scale-aware labels; rows stay playable in size.
- Saved performance presets retain thickness/range and older presets remain compatible.
- Voice leading passed 26,880 transition checks; multi-pad latch stacking remains future work.

## Android Part writing

- Voice lead is renamed Smooth voicing. A separate Part writing checkbox takes
  precedence and searches wider base-chord voicings for fewer parallel perfect
  intervals, appropriate tendency-tone resolution and independent voice movement.
- Selected root/register and explicit inversions remain authoritative. Voice-count
  changes fall back to smoothing; optional routed bass participates in rule scoring.
- Thickness remains octave doubling after base voicing. Touch/controller chord
  histories are separate. Saved presets retain the option; old presets keep it off.
- This is a local preference mode, not a strict SATB/harmonic-rewriting engine.
