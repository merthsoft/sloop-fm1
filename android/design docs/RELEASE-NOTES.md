# Latest firmware — 2.5 Merthsoft.6

October 9, 2026. Native STEP editing now supports continuous tie/rest painting.
Select a note or chord, hold OCT+, and turn STEP clockwise to tie each following
step. Hold OCT− instead to write rests. The source remains intact, painting stops
at the pattern end, backward movement does not erase, and one gesture has one undo.
The controls are inactive for recording/armed/free-take entry and drum tracks.

- Hold either OCT button and turn knob 2 to move the selected note/chord with its
  following ties, microtiming, conditions and locks. Stopped-only, boundary-clamped,
  collision-protected, with complete one-gesture undo/redo.
- New RECORD page: SNAP TRACK/1/8/1/4 records on a coarser grid without altering
  playback DIV. Per-track preference until reboot; incompatible/finer grids show
  DIV LIMIT and use the playback grid. Existing free-take conversion is unchanged.
- 24 musical starters, including twelve new gospel, disco, garage, Latin, ascending,
  descending and two-chord patterns. CHR tracks start the browser in Major without
  changing track scale; CHR remains explicitly selectable.
- Browser PITCH page: independent octave, root, scale and optional VLEAD. Live chords
  and starters share the existing nearest-inversion policy. Preview and apply use
  the same deterministic voiced notes; bass roots remain unaffected.
- Firmware designs move to `docs/firmware`; README introduces the one-minute native
  workflow and Android workspaces with fresh October 9 phone screenshots.

Merthsoft.5 also separates BASS and ARP NOTES starter rendering: BASS keeps lower
roots on the original rhythm; ARP adds eighth-note pulses and cycles triad/seventh
tones while retaining syncopated attacks. Merthsoft.4 added musical starters,
24 drum grooves, rhythm shaping and lossless font packing.

The real UI/audio regression covers painting across banks, source preservation,
step flags/levels, backward movement, end stops, independent gestures, undo/redo,
normal cursor wrapping, octave controls on other pages, navigation and sequencer-fed
arp mode selection. Bounds/arithmetic/shift-count sanitizer checks pass with the
existing DSP negative-left-shift convention excluded (`-fno-sanitize=shift-base`).
Full undefined-behavior instrumentation diagnoses existing negative shifts in
the analog and FX renderers; this release does not alter those DSP paths.
All 24 pure generators and fixed POP FOUR voicing oracles pass full UBSan. The broad
sequencer suite passes coarse-snap lifecycle and retained live voicing/recording cases.
Native browser regressions pass Major initialization, octave isolation, preview,
page navigation, stopped/hold guards and complete replacement undo/redo. All production scales are checked for distinct, bounded voiced notes; sparse-scale inversions that collapse voices are rejected. The broad UI/audio regression and 20,000-frame fuzz pass.

Package SHA-256: `306caa98dd5889239256256eb7ad90c1f9858316d05cbb8f59193e518bfacf1d`.

Target build passes: image 571,472 bytes; RAM 97,348/98,304; pool 333,948/344,064;
RAM-text 925 instructions with no calls; HAL register-access check clean.
Package: `build/update-2.5-merthsoft.6/firmware/sloop-2.5-Merthsoft.6.fwsc`,
610,066 bytes, `FM-1_900`. No hardware flash is claimed for this release.

## Earlier release — 2.5 Merthsoft.3

October 9, 2026. The 16 punch-in effects now have six categories of black-key controls
while FX is held or its layer is locked. White-key effects retain their original IDs.

| Physical key | Control |
| --- | --- |
| F♯3 / G♯3 | Slower / Faster |
| A♯3 | Triplet |
| C♯4 or C♯5 / D♯4 or D♯5 | Gentle / Extreme |
| F♯4 or F♯5 | Blend |
| G♯4 | Latch toggle |
| A♯4 | Retrigger |

Most controls are momentary; latch survives releasing the white key and FX. Hold FX and
press G♯4 again to unlatch; STOP clears latch. Retrigger fades to dry, recaptures/restarts
using the effect's normal transport alignment, then fades back. Opposing rate/strength
keys cancel. Repeated modifier keys remain effective until all copies are released.
Tiles show the adjacent black-key mapping, and the header/LEDs show latch state.
See the [guide](../../GUIDE.md#black-key-punch-modifiers-25-merthsoft3) and
[component design](../../docs/firmware/PUNCH-FX-DESIGN.md) for interaction and DSP details.

- [Local installer](http://127.0.0.1:8789/webapp/installer/).
- Package: `build/update-2.5-merthsoft.3/firmware/sloop-2.5-Merthsoft.3.fwsc`, 610,066 bytes, `FM-1_900`.
- SHA-256: `9be5c106a93633cca50e1ebd2248541b67739f70ad46350a1db393f39c89ebea`.
- Image 579,492 bytes; RAM 97,204/98,304; pool 333,948/344,064.
- RAM-text 925 instructions/no calls and HAL checks pass. Splash retained.
- 160 effect/modifier combinations pass bounded audio, audible-control and exact dry-cleanup checks.
- Real keyboard/UI tests cover latch/retrigger/STOP, duplicated modifier ownership, no note
  playback/recording and 20,000-frame fuzz; undefined-behavior trap instrumentation passes.
- Physical/remote FX priority tests pass. 182 existing golden audio renders are unchanged;
  voice/routing, health and host CPU checks pass.

No physical firmware flash is claimed. The existing ISR static-budget and unavailable ASan
runtime limitations remain recorded in [verification](VERIFICATION.md).
See [previous release](archive/releases/RELEASE-2.5-Merthsoft.2.md).
