# Persistent keyboard scale lights

Hold physical SEL (SLOOP label SCL), then press OCT+ to toggle the guide.
A SCALE LIGHTS ON/OFF message confirms the change.
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

## Brightness

Hold HOME to open the main menu, select LIGHTS, then turn knob 4, SCALE LEVEL.
DIM / LOW / MID / HIGH select 4 / 8 / 16 / 32 microsecond guide pulses. DIM is the
legacy brightness. OCT+ cycles levels while that row is selected. The separate
SEL+OCT+ toggle enables/disables the guide. Played notes keep the normal bright
plane; background LIGHTS/KEYS preferences remain independent.

The adjustable pulse is used only when the persistent scale guide owns the normal
playing keyboard, including held vibrato/tremolo/wah/pan panels. Other layers,
drums and grid landmarks retain their original four-microsecond dim pulses.
The existing timer scan uses precomputed ticks; no new timer or LED plane is added.
Hardware perception and scan-load testing remain user validation.

The fork settings side record stores enabled in bit 0 and level in bits 3–4;
missing bits default to disabled/DIM. Canonical upstream settings keep their format.
Changing their visualizer preserves the local brightness. Menu changes save on
close, with writes deferred while playback runs.

`tests/scale_lights_test.c` checks every root/scale, bright-note priority,
layer/drum/grid isolation, physical toggling, all four pulse levels, menu bounds,
cycle behavior, side-record round trips, legacy defaults and canonical-settings
restoration. The full menu/screen and 20,000-frame UI fuzz suite checks the fourth
LIGHTS row. No song layout changes are required.

The tapped SEL screens show the normal scale sketch when CHORD is OFF and the
physical modifier legend when CHORD is ON. Neither graph changes playing behavior.

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
