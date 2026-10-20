# Factory preset loudness audit — 2026-10-19

The current MerthSloop working tree preserves upstream 2.7.1 normalization.
All **153 factory phrase renders** and **612 controlled renders** were byte-for-byte
identical to upstream commit `4ec1ac829fd2bdf2c571ea215fb3d62477d01220`.
Uneven loudness is reproducible in upstream; this audit found no fork bypass.
No factory gains, envelopes, UI, version, or release artifacts were changed.

## How normalization works

* `tests/preset_level.c` renders each sound through the real voices, sends and master.
  Preset names select bass, held four-note chords, comping, melody, or one-key chords.
  Velocities vary by role (90, 95, 100); each segment is about 4.75 seconds.
* `tools/level_presets.py` measures gated K-weighted loudness on a **mono fold**,
  aiming for -15 LUFS, or -16 for held sounds. Repeated render/generate cycles add
  corrections to `firmware/src/preset_trim.h`. This is offline calibration, not AGC.
* `ui.c:apply_preset_to` assigns the trim to `P_ED_FX`; hostsim mirrors this.
  `fx.c:mix_part_run` indexes `LEVEL_Q12` using `LEVEL + TRIM`, clamped to 1..127
  (LEVEL zero remains silent). Factory loading preserves the track's LEVEL.
  User sounds/projects retain their stored parameters, rather than being remeasured.
* The generator, trim table and runtime gain expression match upstream. Relevant
  fork changes include DSP inlining, equivalent sample decoder factoring, table
  packing, and live modulation/filter handling. With modulation inactive/defaults,
  actual rendered output matched upstream exactly.

## Measurements

Host DSP only, Zig 0.13.0 `cc -O2`, 44,100 Hz, default master/mix and factory FX.
Upstream sources were extracted separately; upstream audio headers were generated
in that isolated tree using the same unchanged CC0 sample assets. Current headers
were read from the existing build; shared generated files were never regenerated.

`tests/preset_loudness.c` forks from a pristine parent for each sound/stimulus to
avoid previous voices, RNG, FX or arena state contaminating the next sound.
Every preset receives velocity 100, a three-second hold and one-second release:
C3 (48), C4 (60), C5 (72), and Cmaj7 (60/64/67/71). Factory mono/legato/poly mode
is preserved; no sequencer or arp clock runs. Chords therefore test actual voice
mode behavior, not necessarily four simultaneous voices for a mono sound.

The full measurements are in [preset-loudness-audit.csv](preset-loudness-audit.csv).
Loudness uses stereo channel energy and 44.1 kHz K-weighting; RMS averages channel
energy. Peak is sample peak, not true peak. RMS windows are attack 0..0.25 s,
sustain 1..3 s, release 3..4 s. Gated integrated loudness includes release.

| C4, velocity 100 | Stereo LUFS | Peak dBFS | Attack RMS dBFS | Sustain RMS dBFS |
|---|---:|---:|---:|---:|
| ACID 303 | -6.75 | -5.21 | -9.55 | -8.69 |
| RHODES | -10.98 | -5.21 | -11.22 | -13.55 |
| SUPERSAW | -11.12 | -5.21 | -13.21 | -13.60 |
| GRAND PNO | -13.66 | -5.21 | -13.13 | -25.91 |
| WARM PAD | -16.85 | -9.70 | -26.90 | -18.71 |
| SOUL ORGAN | -18.40 | -13.09 | -19.48 | -20.39 |
| MUTED GTR | -26.80 | -5.21 | -22.24 | -47.04 |
| BANJO | -27.98 | -5.21 | -21.90 | -35.64 |
| REGGAE ORG | -30.76 | -5.21 | -25.11 | -72.08 |
| CHIP NOISE | -33.62 | -5.21 | -21.00 | -69.98 |

Across C4 sounds, the 10th/50th/90th loudness percentiles are -21.88/-15.18/-9.86
LUFS. For Cmaj7 they are -19.53/-12.74/-10.81. Comparing a single note on a
held-chord-calibrated organ against a melody-calibrated lead is inherently uneven.
SOUL ORGAN measures -18.40 on C4 versus -13.13 on Cmaj7; mono/legato
sounds do not get the same chord gain. Pitch/filter/keyboard response matters too.

Using the **original upstream mono measurement**, factory phrases span -31.00
(CHIP NOISE) to -14.76 LUFS. REGGAE ORG is -27.16, MUTED GTR -23.99, BANJO -22.73,
RHODES -17.13. This reproduces the problem even under the calibration's own stimuli.
Do not compare these mono figures directly with the stereo table: two identical
channels alone add 3.01 LU relative to one mono channel.

## Likely causes and limits

1. **Boost ceiling:** 35/153 active factory trims equal +23 half-dB steps (+11.5 dB).
   Several remain well below target. Default LEVEL is 104, leaving only 23 table
   entries of gain headroom. Raising the trim value alone cannot fix this.
2. **Envelope intent:** REGGAE ORG and CHIP NOISE have zero sustain; MUTED GTR and
   BANJO are decaying physical sounds. A long held-key RMS comparison exaggerates
   their difference from pads. Their short attacks can still reach the output knee.
   Equal peaks are demonstrably not equal loudness.
3. **Stimulus mismatch:** name-based roles compare different pitches, velocities,
   note densities and polyphony. For example RHODES is calibrated on chord comping,
   while REGGAE ORG is classified as melody. STRING STB is classified held through
   the substring `STR`, despite its stab name. These merit listening review.
4. **Mono fold:** stereo FX cancellation biases calibration. SINE PAD C4 loses
   another 2.12 LU in the mono fold beyond the ordinary stereo/mono 3.01 LU offset.
5. **Sample rate:** the legacy generator uses the 48 kHz reference K-filter
   coefficients at 44.1 kHz. The audit designs those filters at the actual rate.
   This is a smaller spectral bias, not an explanation for 10–20 dB disparities.
6. **LEVEL dependence:** high LEVEL clamps positive trims, reducing compensation;
   loading a preset preserves LEVEL. Stored projects/user presets, effects and live
   modulation can change real-device comparisons further.

No hardware was used. These results exclude DAC/headphone response, speakers,
physical controls and hardware timing. LUFS is a useful comparison, not a listening
verdict for isolated instruments; the three-second hold can miss very slow motion.
No conclusion about an individual saved hardware project follows without its data.

## Practical fix plan

1. Keep these measurements as the baseline; first compare freshly loaded factory
   sounds on hardware at the same track LEVEL, master, velocity and routing.
2. Replace name heuristics with reviewed calibration roles, covering intended
   register, single-key/chord use and envelope class. Retain intentionally short or
   atmospheric sounds; do not force their sustain to equal a pad.
3. Correct the generator's sample-rate weighting and measure stereo energy.
   Introduce explicit output/report options, headroom/cap warnings and dry-run mode
   before regenerating anything. Changing mono to stereo requires a corresponding
   target review; keeping -15 blindly would shift the whole bank.
4. Recalibrate using representative phrases plus these controlled stimuli. For
   capped quiet presets, consider lowering the common target before adding gain.
   If more gain range is needed, decouple preset gain from LEVEL's clamped table,
   with overflow/headroom, mute and mixer-glide tests. Do not simply raise the cap.
5. Listen and test chords, rapid retriggers, dense multi-track mixes, FX tails,
   peak/true-peak safety, mono compatibility and LEVEL extremes. Apply reviewed
   changes in a separate sound revision; old saved scenes need an explicit policy.

## Reproduction and validation

Build `tests/preset_level.c` and `tests/preset_loudness.c` with the usual host
include paths (`-Ibuild/gen -Ifirmware/src -Ifirmware/hal`) and `-lm -lpthread`.
Run the former with WAV/list/mono-raw filenames, and the latter with an existing
output directory. Then:

```
python tools/audit_preset_loudness.py RAW_DIRECTORY PRESET_LIST OUTPUT.csv
python tests/preset_loudness_test.py
```

The analyzer requires NumPy/SciPy and never writes trims. Three numerical tests
pass: calibrated 1 kHz tone/gain response, stereo/antiphase channel-energy handling,
and silence/release gating. Actual DSP comparison: 153/153 mono phrase segments
and 612/612 stereo controlled files identical to upstream. Local binaries, raw
renders, isolated upstream sources and dependencies remain under
`build/normalization-audit/`; no shared builds, goldens, commits or pushes changed.
