# Persistent keyboard scale lights

Introduced in 2.5 Merthsoft.8; behavior below includes the Merthsoft.9 alignment
and backlight fixes. Hold physical SEL (SLOOP label SCL), then press
OCT+ to toggle the guide. A SCALE LIGHTS ON/OFF message confirms the change.
This consumes the octave press and SEL page tap, and cancels that hold's latch
shortcut. It does not change pitch, octave, chord state or active note ownership.

The guide, held SEL labels/tiles and held SCALE knob start from the selected synth's
ROOT/SCALE, matching the tapped SEL page. Drums use synth 1 as their held-display
fallback. Held SEL + key and held SCALE knob changes retain their all-synth broadcast
behavior; the tapped page remains per-track. Physical key 0 = F3, across all 27 keys.
Scale changes update it automatically. CHR includes every pitch class; no scale
quantization is enabled by turning on lights. TRANSPOSE and chord keyboard mapping
do not reinterpret the guide's physical pitch classes.

On synth tracks in the normal playing layer, scale keys use the dim LED plane and
pressed notes retain priority in the full-brightness plane. This works independently
of the menu NOTES and background LIGHTS preferences. While this guide is active,
generic KEYS backlighting is suppressed so off-scale keys stay dark unless played.
Turning it off restores the existing backlight preference. Drum/grid and held/locked
control layers keep their established step, effect, mute, roll and scale indicators.

The device preference uses bit 22 of the existing persisted lights word, after the
five-bit visualizer ID. Older settings default to off. Existing settings and project
layouts are unchanged; writes use `settings_later`, deferred until playback stops.

`tests/scale_lights_test.c` drives production controls with audio running, checks
every root/scale mask, normal bright-note priority, layer/drum/grid isolation,
one toggle per press, octave preservation, consumed page tap, settings round trip
and cancellation of the SEL long-hold latch while notes remain held. Merthsoft.9
also covers deliberately mismatched track keys/scales and every background-light
mode/level. The full UI
regression and 20,000-frame fuzz also pass. Target image grows by 240 bytes;
static RAM remains 97,412/98,304 bytes. Merthsoft.9 adds another 112 image bytes.
Physical LED brightness awaits user testing.

Merthsoft.12 preserves the normal keyboard lighting while LFO/vibrato or
ENV/tremolo is held: selected-scale dim lights, bright played notes and generic
backlight preferences retain their normal playing behavior, with no grid landmarks.

## Octave handoff for ordinary keyboard notes

A physically held plain note lights the key that maps to its captured pitch in the
current octave, even with NOTES display disabled. Hold the original key, change
octave, then press the newly lit key before releasing the original. The overlapping
keys share one held note: no envelope retrigger, no extra MIDI note-on, and no
note-off until the final holder releases. This works in both octave directions;
you can also release the new key first. Releasing everything and pressing again
remains a normal new attack. Other synth tracks have independent ownership.

This handoff applies to ordinary keyboard playing (including manually held chords),
with generated CHORD and ARP modes off. Their root/chord/latch behavior is unchanged.
The scale guide remains dim; held/mapped note keys are fully lit.

`tests/keyboard_handoff_test.c` drives real panel/audio frames and checks both
release orders, octave directions, voice age/gates, balanced MIDI output,
reattacks and track isolation. Sequencer, scale/MIDI and broad UI regressions pass.
