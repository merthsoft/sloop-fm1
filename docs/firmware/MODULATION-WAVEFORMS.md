# Shared modulation waveforms

The stateless `modulation_wave(wave, phase, held)` function in `dsp.c` evaluates
control-rate sine, triangle, rising saw, square and held-random values. It accepts
a full unsigned 32-bit cycle and preserves existing fixed-point waveform values,
phase orientation and square polarity. It does not advance clocks or RNG state.

Consumers are the saved track LFO and its graph, live vibrato/tremolo/wah/pan
and their plots, and shaped DRIFT. Automatic MORPH already consumes DRIFT's
single per-track value; it has no additional waveform generator. Track LFO S&H
receives its caller-owned held random value. Smooth random DRIFT retains its
interpolation between private random destinations, which differs from S&H.

Audio oscillators and sample-rate effects retain their shared DSP primitives
and specialized anti-aliasing, pulse duty and hot-loop paths. They do not call
this control-rate dispatcher per sample. Clock domains, sync divisions and
random draw ownership stay intact.

## Controls

On **LFO → DRIFT**, MODE is OFF / RAND / SIN / TRI / SAW / SQR. RAND retains
the original mode value 1. CYCLE, AMOUNT and TARGET are unchanged. Select
TARGET MORPH to move around the manual A/B blend; store compatible captures
first. SIN/TRI sweep back and forth, SAW rises then resets, and SQR alternates
extremes. Square and saw edges are intentionally discontinuous. Shape changes
fade the previous mode through zero before engaging the next. OFF fades out
using the last shape. Timed motion continues on STOP; synced motion pauses.
Repeating shapes use transport phase; RAND retains hold-until-boundary behavior
when joining a different synced cycle.

The per-track shape byte occupies existing padding: no additional static RAM,
oscillator, audio buffer or per-voice state. These runtime settings still reset
on restart; there is no persistence-format change.

## Measurement and validation

Sharing control-wave selectors and plots alone reduces the same .94 target
from 584,308 to 584,132 B: **176 B recovered**. The combined .95 shaped-motion
image is **584,192 / 584,336 B**, leaving **144 B flash**. It is **116 B smaller**
than .94 including new modes and version-string packing. Main static RAM stays
97,548 B, low RAM 28,928 B and pool 335,424 B.

`modulation_wave_test` checks 600,000 frozen waveform comparisons, plot parity,
all 19 MORPH cycle choices for each repeating shape, mode/off fades, paused sync,
RNG isolation, actual knob access and unchanged per-track size. UBSan passes.
Existing random-drift and snapshot tests retain their contracts. Split scenes
include synced sine morphing with incompatible LFO timing and square vowel
morphing alongside TAPE. Hardware timing remains unqualified; helper sharing
is not a measured silicon speedup.

All 200 audio golden renders are unchanged. Both 14-scene × 13-mask split
suites pass with a delayed worker and with forced worker timeout. Target DSP,
inlining, LOFI hot-loop and byte-exact startup restoration guards pass. The
refreshed browser preview matches native audio, and Pages packaging checks pass.

Private target comparisons and logs are in `build/motion-shapes-95/`.
