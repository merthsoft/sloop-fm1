# Upstream SLOOP 2.6 integration assessment

Historical integration record. Main now tracks upstream 2.7.1; see the
[current assessment](UPSTREAM-2.7-INTEGRATION.md).

Original 2.6 base: `4365a1d0075b9194de1c44826325abbd39751bc1`, following `d30d655`.
At that checkpoint, local main and origin/main matched upstream `8fad1a1`; its five subsequent web
commits are integrated on `merthsloop`. The native firmware remains **2.6 Merthsoft.10**.
Source and protocol conflicts are resolved. The combined package builds with guarded
hot DSP inlining, passes audio/functional checks and is served at
http://127.0.0.1:8815/webapp/installer/. Hardware flashing/timing was not tested in
this integration session. See the current release notes for package identity/hash.


## Integration checkpoints

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

- Adopted upstream’s eleven black-key effects, for 27 physical punch effects.
  The fork’s former rate/triplet/intensity/blend/latch/retrigger modifiers are removed
  by user choice. Remote controls negotiate sixteen or twenty-seven effects, preserving old-device compatibility.
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
checks. This is a build comparison, not an upstream full-test-suite result. The first combined target link exceeded XIP capacity by 22,804 bytes and static
RAM by 5,844 bytes. Memory fitting is active; these figures describe the initial
link, not a releasable build. Feature/state sizes are not additive because branches
share code and buffers. Retain our visualizer union optimization and removed styles;
check linker limits after each integrated group. No combined package is released.

The initial three-way comparison found conflicts in these 21 files; those
conflicts have now been resolved.
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

## Integration regression results

The merged host UI harness passes new upstream MIDI/learn/filter/insert/texture/27-effect
checks and 20,000-frame fuzzing. Targeted EDIT tests pass guide-versus-keyboard mapping,
whole-pattern pitch, readout sessions, undo/redo, pitch bounds, ties and track isolation.
Native starter UI tests pass retained cross-track preferences, independent rhythm
selection, matching preview/apply, non-destructive preview and physical apply holds.
These results do not certify hardware timing; linker/build capacity checks pass independently.

Persisted visualizer IDs (five bits at 17–21) and scale/MIDI mapping flags (22–23)
remain intact. New channel filters occupy 26–30 and marker bit 31 identifies the
fork’s new settings layout. Unmarked words keep old fork semantics; upstream’s
unmarked channel bits at 21–25 cannot be unambiguously distinguished from old fork
flags. Re-select channel routing after importing upstream settings if necessary.

HOME menu has a separate MIDI SCALE section (knob 1); SYSTEM retains calibration,
MIDI LEARN CLEAR and ABOUT. Normal LFO/ENV page navigation remains available through
taps, alongside held/locked performance modulation.

## Completed capacity fitting

Factory drum parameters now use a lossless dictionary, saving 5,158 raw ROM bytes
before decoder code; six stable per-voice records cost 132 RAM bytes. All 512 sound
records reconstruct exactly; editable SYN storage and all 32 factory download wire
payloads retain their existing expanded representation. The full audio render sweep
matches all 182 reviewed fixtures. The single LOFI fixture update retains source velocity; forcing only velocity 100 reproduces the upstream hash.

Default boot draws the firmware title instead of the bitmap splash. The combined
image is 581,060 bytes (504 free); static RAM 94,500/98,304; pool
333,900/344,064. Fully overwritten scratch uses 8,800 bytes in spare driver RAM,
leaving 12,820 bytes; linker and build checks enforce an 8 KiB reserve.

-Oz saves cold code while DSP_INLINE preserves hot helpers. All 25 monitored
functions retain their -Os loop call-site counts. A new target test guards this.
Four historical weighted instruction baselines were reviewed against -Os and
refreshed for branch layout/upstream ISR changes; raw loop instruction counts and
call counts remain comparable. These static heuristics are not cycle measurements.
See [optimization notes](OPTIMIZATION.md).
