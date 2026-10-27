# Track sound snapshots and morphing

The native EDIT → MORPH page keeps two parameter states, A and B, for each
synth track. It is a performance tool for changing the sound of an existing
part while its notes keep playing. It does not capture or replace a sequence.

## Controls

- Knob 1 MORPH: 0% is A, 100% is B. Continuous parameters interpolate in their
  native control units; named choices switch at the midpoint.
- Knob 2 SET A: turn right twice to capture the current sound as A.
- Knob 3 SET B: turn right twice to capture the current sound as B.
- Column 4 STORED: EMPTY, A, B or A+B; read-only.

Capture A, edit the sound on the other pages, capture B, then turn MORPH.
Each track keeps its own pair when switching tracks or leaving the page.
Both endpoints are required before morphing. Capture and morph work during
playback and recording without disarming the track.

![Native track morph page](../../assets/screens/merthsoft/neon/track-morph.png)

## Scope and lifetime

Captured controls are level, envelope and its modulation destinations/trim,
LFO and its destinations/sync, eight engine values, distortion/chorus/delay/reverb
sends, glide amount, pan, unison detune, track filter, insert and ANALOG filter type.
The canonical track descriptors classify common controls; in particular envelope
times remain numeric even when ENV SYNC displays a list of divisions. Engine
selectors are visited before their dependent fields, whose dynamic labels can
identify named controls such as the LOFI wave table or PHYS chord choice.

Pattern notes/ties/locks, timing, root/scale, transpose, chord/arp routing, mute,
voice mode and allocation are excluded. Global effects, live punch settings,
DRIFT/EVOLVE/SUSTAIN runtime settings, uploaded audio and edited FM6 operator
payloads are not duplicated. An engine's existing sample/patch selector is an
ordinary discrete engine control, using its existing loader/polling behavior.
This is parameter interpolation, not an audio crossfade between two engines;
discrete changes can be abrupt.

Loading a factory/user sound or adopting a project/section invalidates the pair.
STOP and track selection do not. Snapshots are RAM-only and reset at power-off.
Saving a section stores the current morphed track parameters through the ordinary
project representation, but does not save the A/B pair. This avoids changing
upstream project formats. Persistent pairs and timed morphs are future extensions.

## Execution and locks

Capture/apply runs in the UI's existing IRQ exclusion, never in a sample loop.
There is no continuously active morph runner. Every explicit knob move computes
from immutable endpoints, so scrubbing back and forth does not accumulate rounding
error. Unchanged endpoints reproduce exact parameter values.

For a currently active parameter lock, capture reads its underlying base if the
instantaneous value is still the lock's value. Morph updates that base while
leaving the lock audible. Leaving the locked step restores the new morphed base.
A knob override follows the existing lock ownership test. Voice state, gates,
MIDI output, RNG and pattern memory are untouched by the operation.

## Space and validation

The pair uses 852 bytes for three tracks × two complete 71-value arrays, plus
24 bytes of per-track controls. Only the documented subset is applied. Target
alignment/global layout determines final occupied RAM.

To fund the feature, ANALOG's previously duplicated LP and BP/HP bodies now share
one out-of-line renderer. Its per-sample helpers remain inline. The target budget
now follows `analog_body` with the original 638 weighted-instruction allowance;
the measured shared loop scores 640 and has zero loop calls. The tiny dispatcher
is not treated as evidence of a speed improvement. Hardware deadline/stack
qualification remains required; host timings do not establish target cycles.

`tests/track_snapshot_test.c` exercises all engines at every morph position,
exact endpoints, dynamic/named controls, missing endpoints, locked bases,
voice/pattern/MIDI/RNG preservation, invalidation, isolation and native controls.
Audio goldens, engine/filter coverage, dual-core/timeout parity and the full UI
navigation/fuzz suite supplement these checks.
