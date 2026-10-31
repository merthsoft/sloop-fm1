# Reverse, console and bowed-string voices

The sound browser includes eight more presets using the existing engines:

| Preset | Engine | Browser kind | Character |
| --- | --- | --- | --- |
| SYN VIOLIN | ANALOG | INST | A bowed synth lead: saw body, filtered noise friction, slower attack and delayed vibrato |
| REV METAL | DIGITAL | PLUCK | Bright metallic FM swell |
| REV HARP | DIGITAL | PLUCK | Rounder reverse-pluck swell |
| REV CHOIR | VOICE | PAD | A swelling, moving vowel chord texture |
| GB RAZOR | LOFI | CHIP | Four-bit double-saw wave-RAM sound |
| NES SPARK | LOFI | CHIP | Stepped pulse pluck with a small falling attack sweep |
| MD CRYSTAL | DIGITAL | CHIP | Arcade-style FM bell |
| MD PICK | DIGITAL | CHIP | Short, gritty FM picked-string sound |

Find them in **SAVE → SOUNDS**, select the kind with knob 2, audition with knob 1 or PRESETS, and turn KEEP right to accept. They are also appended to their engine-local preset lists. Existing preset indices, browser positions and favourite identities remain intact.

Hold reverse voices through their swell, or give sequencer notes enough length. They rise from silence and close after the crest even while held; ENV → ATK controls the rise. These are synthesized reverse-style envelopes, not reversed recordings. The earlier REV LEAD, REV GLASS, REV STR, REV HAZE and REV REED remain available.

SYN VIOLIN takes a different approach from PHASE VIOLIN: actual filtered oscillator noise adds bow-like friction, a roughly 50 ms attack avoids the sharp horn-like onset, and gentle delayed vibrato adds movement. It is a synthetic string lead rather than a sampled acoustic violin. PHASE VIOLIN retains its existing identity so saved sounds do not silently migrate between engines.

## Implementation and verification

The batch appends one ANALOG, four DIGITAL, one VOICE and two LOFI recipes. This pack brings normal factory count to 192; the [horror voices](HORROR-SOUNDS.md) extend it further. Optional SLICE adds four. No oscillators, engines, samples, tables or audio buffers are added. These sounds still incur their chosen engine's ordinary per-voice processing; using an existing engine does not make every preset equally cheap.

Measured half-dB trims are SYN VIOLIN -6, REV METAL 9, REV HARP 12, REV CHOIR 0, GB RAZOR 7, NES SPARK 16, MD CRYSTAL 12 and MD PICK 21. Existing trims and recipes remain unchanged. Standard phrase loudness ranges from approximately -16.35 to -14.48 LUFS, with held-pad material measured against the existing quieter held-chord target. Instrument attack and crest differences remain intentional.

`expanded_sounds_test` checks all eight names, trims and distinct recipes, pitches C3–C6, velocities 40/80/120, bounded dry audio and clean release. Isolated auditions additionally covered bowed-string range/velocity cases, reverse crest and early release, and console phrasing. All 192 factory records round-trip through startup packing; the 184 preceding records remain byte-identical. Existing audio goldens remain unchanged, with eight new reviewed entries.

Private listening and measurement evidence is under `build/violin-107/`, `build/reverse-expansion-107/` and `build/console-expansion-107/`. The combined MORPH/sound-pack target budget is recorded in [headroom](HEADROOM-COMPARISON.md).
