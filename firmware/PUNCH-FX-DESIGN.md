# Punch-in FX black-key controls

Implemented in **2.5 Merthsoft.3**, October 9, 2026. The 16 white-key effect IDs and
remote performance protocol remain unchanged. See [the playing guide](../GUIDE.md#black-key-punch-modifiers-25-merthsoft3)
for the physical mapping; octave numbers use the FM1 labels.

## Interaction and ownership

The six control categories are rate (Slow/Fast), Triplet, intensity (Gentle/Extreme),
Blend, Latch and Retrigger. Six categories require eight distinct buttons; the upper
C-sharp/D-sharp/F-sharp keys repeat Gentle/Extreme/Blend. Black-key modifiers use the
existing FX keyboard owner and never emit or record synth notes. A physical key mask
keeps duplicated momentary controls independent. Opposing rate/intensity controls cancel.

A white key selects the local effect. G-sharp4 toggles latch only with a selected effect;
its release is not another toggle. Releasing the owner key or the FX layer keeps a latched
effect active; selecting a new white key replaces it. Unlatching with no owner key held
requests dry audio. STOP and panic clear local latch, request and modifiers. Local latched
effects retain physical priority over remote leases; remote cleanup cannot release them.
Layer exit clears momentary controls, including keys still physically held across exit.
Latch is a RAM performance state and is not saved in projects/settings.

## DSP

The existing mono ring and stereo DSP are reused: no new delay/sample buffers. Active
FX calculate coefficients and modifier timing once per audio block; idle mixing bypasses
modifier calculations. Loops/reverse/half speed use fractional playback phase. Slow/Fast
shift pitch together with playback speed; no pitch-preserving stretch is claimed.
Triplet changes retained loop length, gate/echo divisions, and wobble rate. Gentle halves
wet level; Blend halves it again. Extreme deepens supported filter/crush/alias/gate/echo/
wobble parameters. On captured loop effects Extreme retains full wet playback rather
than adding gain. Neutral effect selection retains the existing effect character.

Rate and strength pairs cancel to neutral. Wet changes use the existing 64-sample gain
ramp. Retrigger first fades to dry, calls normal effect initialization/capture (including
running transport grid alignment), then fades in. Echo feedback remains bounded and its
ring writes saturate. Normal release, STOP and unlatch return to exact dry audio after
fade; STOP does not leave a wet tail owned by latch.

## Verification and limits

`tests/punch_test.c` covers 16 effects x 10 modifier combinations: audible changes,
opposing controls, bounded audio and exact dry cleanup, plus real keyboard latch,
release, duplicate-modifier ownership, retrigger and STOP. `tests/ui_pages_test.c` covers
FX LEDs/caption, layer release, unlatching and 20,000-frame fuzz on the real UI/audio.
`tests/performance_engine_test.c` covers physical/remote priority and cleanup.
182 broader audio golden renders, voice/routing and host CPU checks remain unchanged.
Trap-instrumented undefined-behavior checks pass. Target image/RAM/pool/RAM-text/HAL
checks pass. Host tests do not establish target audio deadline margin; the historical
ISR cost baseline warning remains recorded in the Android verification document.
