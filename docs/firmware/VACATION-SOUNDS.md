# Vacation sound collection

Twelve additional factory presets cover four themes. They reuse existing engines: no new DSP, sample data, audio buffers or performance controls.

| Theme | Sound | Engine / browser kind | Playing idea |
| --- | --- | --- | --- |
| Space after dark | ORBIT PAD | PHASE / PAD | Hold wide minor-seventh chords for a gently moving, detuned backdrop. |
| Space after dark | STAR KEYS | DIGITAL / KEYS | Bright FM keys with a softer tail; try sparse upper-register chords. |
| Space after dark | LUNAR VOX | VOICE / PAD | Airy, rounded synthetic choir for long chords and drone layers. |
| Travel postcards | ISLAND PAN | PHYS / PLUCK | Modal struck-metal voice; short melodies and overlapping notes. |
| Travel postcards | DESERT OUD | PHYS / PLUCK | Damped string plucks with a woody attack; try low drones and ornaments. |
| Travel postcards | DUSK ORGAN | WHEEL / ORGAN | Warm, slowly opening organ with gentle rotary movement. |
| Arcade souvenirs | PIXEL BASS | LOFI / CHIP | Short stepped-envelope pulse bass; repeated low notes work well. |
| Arcade souvenirs | SAVE BELL | LOFI / CHIP | Bright wave-RAM bell with a short decaying body; sparse high notes. |
| Arcade souvenirs | CRT LEAD | PHASE / CHIP | A sharp electronic lead with delayed vibrato; play a simple melody. |
| Strange little toys | GLASS RAIN | GRAIN / PAD | Small scattered fragments from the factory vibes sample; hold a few notes. |
| Strange little toys | TOY ORGAN | WHEEL / KEYS | Thin upper harmonics and a percussive contact sound; short chord stabs. |
| Strange little toys | ROBOT TALK | VOICE / FX | Fast formant movement and breath for talking-machine phrases. |

Open **SAVE → SOUNDS**. Knob 2 selects the browser kind; knob 1/PRESETS browses and auditions; turn knob 3 right to keep a sound. Each addition also appears at the end of its engine's preset list. **DIGITAL now has 32 presets.** Existing sounds and their order are preserved.

Preset browsing preserves existing track effect and voice-mode preferences. For the intended pad chords, use POLY. If a sound is unexpectedly dull, check the global low-pass filter; existing effects can also change its character. These are synthetic interpretations rather than recordings or exact recreations of particular instruments or consoles.

## Implementation and validation

The collection adds twelve recipes and browser entries, bringing the normal factory set to **208 sounds**, plus four optional SLICE presets. Existing preset recipes, indices and calibrated trims remain unchanged. New sounds receive individual level trims; short chip bass and sparse granular sounds deliberately have lower integrated phrase loudness than sustained pads.

The ten-entry startup WHEEL preset table moves from low RAM into spare pool RAM (360 B). The main startup literal budget changes from 6,250 to 6,144 B to preserve main-RAM margin. Both restore only at boot; no decoding is added during playback. The existing 8,192 B pool reserve remains intact.

The hardware image is **583,844 / 584,336 B**, leaving **492 B flash**. Main static RAM is **98,160 / 98,304 B** (144 B free), low RAM **29,324 / 29,392 B** (68 B free), and pool **335,784 / 344,064 B** (8,280 B free including the reserve). Compared with the horror-preset checkpoint, this costs 320 B flash.

Checks cover every addition at MIDI 48/60/72/84 and velocities 40/80/120 for distinct recipes, audible bounded output and clean release. The longer pads are allowed their designed release tails. All **237 audio goldens pass**, including exact preservation of the preceding 225. All 208 raw and startup-restored factory records match, and the frozen editor descriptor corpus is unchanged. Target literal relocation, guarded repeated startup restoration and UBSan checks pass. Actual hardware audition remains the final judge of tone and load.

Private firmware-generated audition clips and check logs are in `build/vacation-109/`. Each theme clip plays its three sounds in the table's order, seven seconds per sound.
