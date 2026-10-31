# Violin bow-edge refinement

The PHASE VIOLIN recipe was unusually smooth: one undetuned SAW with DCW 38 and ENV 12. Its steady tone had very little upper-harmonic energy. Increase DCW to 82 and ENV to 26, retaining the same waveform, envelope, mono mode, sends, trim and render path. The sustained tone gains bite and the attack has a stronger brightness contour.

This is a synthetic bow-edge treatment using existing phase distortion. It does not add recorded bow noise or a physical bowed-string model. Hardware listening remains the final assessment of how convincingly the texture evokes a violin.

## Measurements

The controlled factory melody measures **-15.155 LUFS before / -14.846 LUFS after** using the existing calibration algorithm. Both recommend the existing **-14 half-dB trim** at its -15 LUFS target; no trim change is needed.

At A4 / velocity 100, the sounding second has:

| Measurement | Before | After |
|---|---:|---:|
| RMS, signed16-bit units |5851.84|5950.70|
| Spectral centroid |461.17 Hz|569.54 Hz|
| Energy above 2 kHz |0.0092%|1.1365%|

Normalized first-difference energy rises **1.935 times**, showing added spectral edge independently of a gain increase. The higher-harmonic measurement is objective evidence of the timbral change, not a perceptual realism score.

A rejected alternating SAW/SAW-PULSE recipe introduced approximately 21% sub-fundamental energy at A4. The retained single-wave recipe avoids that octave ambiguity and retains the existing cheap single-line path. No new DSP, state, parameter IDs, allocations or rendering branches were added. Parent measures the combined target image; no independent linked flash saving/cost is claimed here.

## Validation and listening

`tests/violin_preset_test.c` uses isolated child processes for the previous/current A4 comparison and 15 current note/velocity cases: G3, C4, A4, E5, C6 at velocities 32, 80, 127. Assertions check increased normalized edge, bounded RMS change, no sample near full scale, freed voices and release-tail silence. All 17 render cases pass. An additional 30 old/new range captures pass; the new maximum peak is 15,114.

Direct-source regression output for VIOLIN is intentionally **4a7723100d71ee0a** with peak 14,525, tail 0, DC -0.701 and voices free 0.0210 seconds after final note-off. The parent reviews/integrates golden expectations. No unrelated golden was changed by this task.

Private reproducible artifacts are in `build/violin-106/`:

- `before.wav` and `after.wav`: the existing factory melodic calibration phrase, with matched trim and sends.
- `violin-before-after.wav`: those phrases consecutively, separated by half a second.
- `loudness.json`, `spectra.json`, `probe.c`, `level.c`, and associated scripts retain measured evidence.

The focused test can be built using the usual host compiler, assertions enabled, `-O2 -fwrapv -fno-strict-aliasing`, generated headers, firmware/hal headers, and `-lm -pthread`.


## Alternative bowed-string voice

Listening feedback identified the PHASE result as still horn-like. The separate ANALOG **SYN VIOLIN** preset uses actual filtered noise, a slower bowed attack, steady filter and delayed vibrato instead. It preserves PHASE VIOLIN's existing saved identity; see [the performance sound pack](PERFORMANCE-SOUND-PACK.md). This remains a synthetic string interpretation.
