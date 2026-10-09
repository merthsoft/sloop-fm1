# Persistent keyboard scale lights

Implemented in 2.5 Merthsoft.8. Hold physical SEL (SLOOP label SCL), then press
OCT+ to toggle the guide. A SCALE LIGHTS ON/OFF message confirms the change.
This consumes the octave press and SEL page tap, and cancels that hold's latch
shortcut. It does not change pitch, octave, chord state or active note ownership.

The guide uses the same song ROOT/SCALE and physical pitch-class mask as the
existing held SEL layer: `trk[0]`, physical key 0 = F3, across all 27 keys.
Scale changes update it automatically. CHR includes every pitch class; no scale
quantization is enabled by turning on lights. TRANSPOSE and chord keyboard mapping
do not reinterpret the guide's physical pitch classes.

On synth tracks in the normal playing layer, scale keys use the dim LED plane and
pressed notes retain priority in the full-brightness plane. This works independently
of the menu NOTES and background LIGHTS preferences. Drum/grid and held/locked
control layers keep their established step, effect, mute, roll and scale indicators.

The device preference uses bit 22 of the existing persisted lights word, after the
five-bit visualizer ID. Older settings default to off. Existing settings and project
layouts are unchanged; writes use `settings_later`, deferred until playback stops.

`tests/scale_lights_test.c` drives production controls with audio running, checks
every root/scale mask, normal bright-note priority, layer/drum/grid isolation,
one toggle per press, octave preservation, consumed page tap, settings round trip
and cancellation of the SEL long-hold latch while notes remain held. The full UI
regression and 20,000-frame fuzz also pass. Target image grows by 240 bytes;
static RAM remains 97,412/98,304 bytes. Physical LED brightness awaits user testing.
