# Track auto-wah and auto-pan — implemented

Hold **ENV + FX** for auto-wah, or **LFO + FX** for auto-pan. Both gestures work
in either press order. They affect only the captured, selected synth track; drums
are excluded. Physical keys continue playing normally and scale lights remain.
Releasing either button stops an unlocked effect and restores the preceding page.
The remaining button is consumed until both are released, avoiding an accidental
normal FX/ENV/SEL screen. Hold the combo and tap **HOME** to lock the panel/effect;
HOME unlocks, and STOP, panic, track change or entering a menu clears it.

## Wah

| Knob | LFO mode | Envelope mode |
| --- | --- | --- |
| 1 | Free rate in Hz; turning it exits beat sync | Input sensitivity, 0–127 |
| 2 | Sweep depth | Sweep depth |
| 3 | Base low-pass cutoff, −64 (dark) to −12 (bright) | Same |
| 4 | OFF = free triangle LFO, or a beat division | ENVELOPE, after the beat divisions |

Beat divisions follow the same choices as vibrato/tremolo, including triplets and
long divisions. Triangle motion opens a resonant low-pass filter from its base without crossing
into high-pass mode or bypassing the resonance. The existing smoothed track filter avoids abrupt coefficient
changes. At zero depth only the chosen base cutoff is active.

ENVELOPE follows the selected synth's own rendered mono signal **before** distortion,
inserts, slicing, filtering, level and sends. Other tracks, USB/master input and
reverb/delay returns cannot drive it. The block peak uses a quick attack and slow
release (about 6 ms / 93 ms time constants); knob 1 adjusts sensitivity. This is
an audio-level follower, not the synth's ADSR. Sensitivity is calibrated for ordinary synth output, rather than full-scale mixer
peaks. Defaults are sensitivity 63, depth 96/127 and base cutoff −48; turn sensitivity
down if it stays open, or up if attacks barely move the meter. The wah has higher
resonance than the normal track filter. The saved track filter and patch
parameters are never overwritten. This path works across synth engines, including
SAMPLE, because it uses the track filter after rendering.

## Pan

Knobs 1–4 are **rate Hz / depth / waveform / beat sync**, like vibrato/tremolo.
The sweep is added around the saved track pan and clamps safely at the stereo
edges. The existing track panner affects the dry track; shared effect returns keep
their normal stereo processing. Use headphones to hear the left/right movement.

Wah and pan are live performance effects. HOME locking lasts until cleared and
is not song persistence. OCT+ currently reports the lock gesture rather than
copying them into a saved destination: the shared LFO has no pan destination,
and engine-filter modulation is not equivalent to the track-level wah follower.
Existing vibrato/tremolo OCT+ application remains unchanged.

## Implementation and verification

Four effects share the existing compact runtime modulation structure and oscillator
clock. No audio histories, project-format changes or new sample buffers are added.
Only active envelope wah scans its own block. Production panel/audio tests cover
both press/release orders, keyboard and scale lights, lock cleanup, track isolation,
follower attack/decay, tempo sync, bounds and unchanged saved parameters.
Physical sound/timing validation remains on the FM1.
