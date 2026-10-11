# Upstream SLOOP 2.6 integration assessment

Upstream main: `4365a1d0075b9194de1c44826325abbd39751bc1`, following the 2.6
implementation commit `d30d655`. Local main and origin/main were fast-forwarded to
this revision. Android remains at `5b80ebf`; this assessment does not merge 2.6
into the custom firmware or replace the running installer.

## Recommended integration order

1. Project format, parameter metadata and backup compatibility together. Upstream
   FUN6 packs ten new parameters into bytes and combines nudge/fill fields; it reads
   FUN5 and older formats. Preserve our complete load undo and test old/new projects,
   parameter locks, saved sounds, working autosave, and cold/warm boot. Update Android
   native parameter IDs, project/backup assumptions and the negotiated web editor;
   engine parameter IDs move because P_E0 follows the ten new parameters.
2. Audio correctness: preset/engine/FM6 patch fade coordination, look-ahead limiter,
   click fixes and FREEZE correction. Carry upstream's new audio goldens only after
   reproducing intended changes; preserve source velocity and USB return mixing.
3. MIDI sustain, bend/range, Program Change, per-track routing, learn and step entry.
   Preserve our remote ownership, fixed-channel/generic-MIDI behaviors, physical
   octave reporting, scale mapping and connection/STOP/panic cleanup. MIDI settings
   add backup object 10, requiring companion backup round-trip coverage.
4. LFO SYNC/TRIG/POL, envelope SYNC, ANALOG LP/BP/HP filter and track inserts.
   Keep tap LFO/ENV navigation plus our held modulation/lock panels. These are saved
   patch features; runtime vibrato/tremolo remain independent. Test dynamic parameter
   descriptions as well as DSP silence, tails, fades and CPU/memory bounds.
5. Expanded punch effects and throws, TEXTURE, dotted-quarter delay, drum mutes and
   note LEN/velocity controls, preserving our OCT-held STEP tie/rest/move/octave
   gestures, starter previews, custom bank/rhythm controls and sequenced arp modes.
6. Font/UI refresh and offline web editor once source/protocol behavior is stable.
   Upstream has 1-bit font packing too, but uses a per-font marker and Bold/capital
   styling. Preserve packed storage; font packing savings cannot be added twice.

## Control decisions and semantic overlaps

- Upstream assigns eleven black FX keys to new effects. Our black FX keys modify
  the sixteen effects (rate, triplets, intensity, mix, latch/reset). A combined UI
  needs an explicit bank/modifier gesture, rather than silently losing either.
- Upstream latched arp remaps held keys as root/scale/transpose changes (#191).
  Preserve our chord modifier toggles, live latch gestures, sequenced SNOTE/SPLAY
  ownership, expression and octave semantics; these need behavioral tests even
  where Git merges without a text conflict.
- Upstream's MIDI channels and LEARN persistence touch device settings and backup
  assumptions. Our protocol 14 commands 73–76 and relocated SYN commands 80–84
  must remain stable; parameter/global counts should come from negotiation.
- The current native independent rhythm bank has no Android selector yet; that
  limitation remains unchanged by this upstream review.

## Measured capacity and merge scope

Same installed Jieli toolchain, AC79 SDK and slice=0, upstream generated assets:

| Build | Image bytes | Static RAM / 98,304 | Pool / 344,064 |
| --- | ---: | ---: | ---: |
| Upstream 2.6, isolated source | 573,052 | 92,696 | 333,900 |
| Current Merthsoft.19 | 579,312 | 96,244 | 333,948 |

Upstream target build passes HAL/register-access and call-free 925-instruction RAM
checks. This is a build comparison, not an upstream full-test-suite result. Combined
capacity is unmeasured: feature/state sizes are not additive because both branches
share code and buffers. Retain our visualizer union optimization and removed styles;
check linker limits after each integrated group. No combined package is released.

Read-only three-way comparison found text conflict markers in these 21 files.
Cleanly merged files can still have semantic incompatibilities (notably project,
editor and core metadata), so this is not a complete risk list:

- `GUIDE.md`
- `README.md`
- `SLOOP.md`
- `build-sloop.ps1`
- `firmware/src/gfx.c`
- `firmware/src/panel.c`
- `firmware/src/params.c`
- `firmware/src/punch.c`
- `firmware/src/seq.c`
- `firmware/src/ui.c`
- `firmware/src/ui_input.c`
- `firmware/src/ui_layers.c`
- `firmware/src/ui_menu.c`
- `firmware/src/voice.c`
- `tests/project_test.c`
- `tests/punch_test.c`
- `tests/seq2_test.c`
- `tests/stress_test.c`
- `tests/ui_pages_test.c`
- `tools/gen_font.py`
- `web/editor.html`

Build evidence is local/ignored: `build/upstream26-build.log`; isolated upstream
source/build under `build/upstream-2.6`. No installation was performed.
