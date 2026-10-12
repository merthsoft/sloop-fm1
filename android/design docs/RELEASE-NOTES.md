# Latest firmware — MerthSloop 2.6.0.12

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
