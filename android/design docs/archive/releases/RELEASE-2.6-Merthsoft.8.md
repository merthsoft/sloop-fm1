# Latest firmware — 2.6 Merthsoft.8

- Hold LFO or ENV and tap OCT+ to apply vibrato/tremolo parameters to the track's
  saved shared LFO. Rate, waveform and beat division transfer; tremolo depth is
  exact and vibrato depth rounds to the regular pitch destination's steps. The
  temporary effect stops to avoid doubling; unrelated destination depths stay.
- LOFI MOTN ARP retains OFF/OCT/MAJ/MIN and adds every SEL chord shape, using the
  existing harmony builder and root/scale. No additional voices or persistent RAM.
- Identical font bitmaps share offsets: 816 bytes less data, unchanged renderer
  and exact pixels for every supported character.

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

Upstream SLOOP 2.6 is integrated into the android branch. Local main and origin/main
match upstream revision 4365a1d; fork functionality remains on android.

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

Target image **581,220 bytes** (344 free); static RAM **94,500/98,304**
(3,804 free); pool **333,900/344,064** (10,164 free). Fully overwritten
scratch uses 8,800 bytes beside the flash driver, leaving 12,820 bytes there;
linker/build checks preserve an 8 KiB driver reserve. Stack guards are unchanged.
HAL and call-free RAM driver checks pass.

Package **609,941 bytes**, identity **FM-1_900**. SHA-256:
`a5b0fee96905d98754df4716f13decb18a3eabb9ec65cb8d87fd40b0a49a04fa`.
Installer: http://127.0.0.1:8813/webapp/installer/

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

