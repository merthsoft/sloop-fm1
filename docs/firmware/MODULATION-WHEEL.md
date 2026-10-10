# Live performance modulation — Merthsoft.14

Hold **LFO** for selected-synth vibrato; hold **ENV** for selected-synth tremolo.
After 140 ms the temporary panel appears and its effect engages. Release dismisses
it and restores the previous page and underlying sound. A quick tap still opens the
normal LFO or ENV pages. The keyboard continues playing normally in either panel.
Hold LFO/ENV and tap HOME to lock the panel and effect, then release the button.
The LOCK indicator appears; keyboard playing and knob edits continue. HOME unlocks
and clears the effect. Another function button unlocks on its first press and
opens its normal page on the next press. One panel locks at a time, for synths
only. Unlock before using the normal panel track-selection controls. The lock is
runtime-only and is not saved in a patch, project or settings. STOP, track panic,
menu entry and selected-track changes also dismiss a locked panel.

Both panels use knob 1 **rate**, knob 2 **depth**, knob 3 **waveform** (sine,
triangle, saw, square), and knob 4 **beat sync**. Vibrato starts near 5 Hz with
approximately +/-12.5 cents of pitch movement; tremolo starts near 4 Hz at 38% depth.
Maximum vibrato is approximately +/-50 cents; maximum tremolo approaches silence
at the low point. Parameters are independent of saved LFO/envelope values and
remembered across holds until reboot, shared between tracks for each effect.
SYNC selects OFF or the existing nine step divisions: 1/4, 1/8, 1/16, 1/32, 8T,
16T, 1/2, 1BAR, 2BAR. OFF uses free Hz; turning knob 1 returns SYNC to OFF.
Playing cycles lock to the existing transport phase, including external MIDI clock.
Stopped cycles run at current BPM. Compile-time phase reciprocals avoid new 64-bit
division in the audio ISR. Saved patch LFO/fade parameters remain untouched.

Each effect captures the selected synth at button-down. Other synths and drums are
unaffected. Switching tracks cancels that hold rather than transferring its effect.
STOP, track panic, reset and menus cancel it; an active effect does not reactivate
until the button is released and held again. Holding both buttons allows both
modulations; LFO has priority for the visible panel and knobs.

Independent oscillators tick once per selected track block, sharing their result
across voices. Tremolo scales voice amplitude before its existing block ramp.
No allocation, new DSP buffer, patch/project parameter or MIDI format is added.
Held, latched and arpeggiated notes are not retriggered by the effect.

MIDI CC1 remains the Merthsoft.10 wheel: it follows normal synth-channel routing
and the saved track LFO rate/wave/fade, with +/-50 cents at maximum. Held hardware
vibrato overrides CC1 only on its captured synth; release restores the latest CC1.
CC120/121/123 reset modulation. IN CLOCK and drums ignore CC1. STOP clears all
wheels; panic resets only that track. USB reset/detach clears USB-owned values,
retaining TRS-owned values. TRS unplug is not detected.

Android Perform's momentary strip still sends/deduplicates CC1, resetting to zero
on release/cancel and lifecycle cleanup. It remains mutually exclusive with the XY
macro, and is not recorded in note-only Perform capture. External-controller input
through the phone does not forward CC1; direct FM1 MIDI can send it. Hardware
vibrato/tremolo do not transmit outgoing controllers.

Host tests also cover HOME lock/release for both effects, playable locked keys,
knob edits, preserved scale lights, function-button unlock consumption, drum
rejection, and dismissal on STOP/panic/track change. Locks add no static RAM.

Host tests cover temporary panels, tap navigation, selected-track isolation,
waveform bounds, saved parameter preservation, held-note gates, incoming USB/TRS
ownership and STOP/panic cleanup. Physical gesture/audible testing remains user testing.

Merthsoft.12 preserves the normal keyboard lighting while LFO/vibrato or
ENV/tremolo is held: selected-scale dim lights, bright played notes and generic
backlight preferences retain their normal playing behavior, with no grid landmarks.

The panels show one modulation cycle with a moving marker driven by the audio
phase. Vibrato shows the current pitch offset in cents; tremolo shows the current
volume percentage. Depth changes the curve excursion. Track, root and scale appear
in the title, and a compact scale-note guide shows accidentals such as F# in E minor.
This is a guide only: physical playing, MIDI mapping and scale lights are unchanged.
The display uses the existing canvas and oscillator helpers; it allocates no history
buffer and never advances the audio phase.
