# LOFI attack pitch sweep

LOFI → MOTN → SWP controls downward pitch depth, independently of the master
LOFI effect. Zero disables the pitch drop. Halfway is one octave, maximum is
23.8125 semitones. Each new oscillator attack starts at the played pitch and
settles downward with an approximately 93 ms time constant (about 0.3 seconds
to 95% depth). Live parameter changes approach their new depth smoothly.

The old implementation added `SWP / 8` to a pitch accumulator every 32-frame
control block. Raw values 0–7 did nothing; raw 8 immediately fell at about
86 semitones per second, with discrete speed jumps at every multiple of eight.
It continued until clamped at the pitch floor. The percentage display obscured
that coarse rate quantization.

The replacement reuses the same per-voice state as Q8 of 1/16 semitone and
approaches `SWP * 768` with a 1/128 control-block step. It adds no RAM, lookup
table, per-sample division, or sample-loop work. The existing pitch conversion
still quantizes to 1/16 semitone. Low MIDI pitches can reach the normal MIDI
pitch floor when asked to drop below it.

## Validation

`lofi_sweep_test` exercises all 128 amounts through the real oscillator render,
checking nonzero low amounts, exact settled depth, long holds, response timing,
live changes, zero and retrigger under UBSan. Of 200 audio golden renders, only
`preset/LOFI/07_CHIP_NOISE` changes: it is the factory render with nonzero SWP.
Its expected hash is updated explicitly; all other 199 hashes remain unchanged.
Native/WASM parity also exercises SWP 1, 8 and 127. Target DSP budgets, inlining,
master LOFI divide/call guards and startup literal restoration checks pass.
Existing SWP settings keep their stored value but intentionally use the new
bounded-depth behavior.

The combined normal **2.8.0.94** target is **584,308 / 584,336 B**, leaving
**28 B flash**. That is 4 B above .93, including the version string's packing
effect. Main static RAM remains 97,548 B, low RAM 28,928 B and pool 335,424 B.
Private comparison artifacts and test logs are under `build/lofi-sweep-94/`.
