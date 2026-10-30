# Drum allocation and factory snare balance

This implements the first two recommendations from the
[drum loudness audit](DRUM-LOUDNESS-AUDIT.md): upstream's shared allocator, then
a restrained three-sound body/transient pass. It does not boost the entire bus,
raise the limiter threshold, add voices or change saved-kit formats.

## Allocation

`drum_pick()` follows upstream 2.8.3 (`main` at `5eb24c1`): a free slot first,
then a matching note, then the oldest non-kick, falling back to the oldest kick.
GM 35 and 36 receive kick protection. Sampled, user-sample, factory-synth and
editable SYN paths share the policy. Existing hat choke runs before selection;
the replaced voice's last output still enters the declick tail.

The six-voice budget is unchanged. With free slots, repeated hits may overlap;
same-note priority applies when full. The upstream age-key behavior is retained.

The audit's controlled dense-groove comparison reduced active kick replacements
from 936 to zero across 148 scenarios per allocator. Simple grooves with no
pressure were unchanged. Preserving kick articulation does not necessarily
increase whole-mix RMS; allocation does not fix quiet isolated sounds.

## Three factory sound edits

Only SNARE in 808, 606 and VINTAGE changes. Per-entry copies prevent inherited
kits from changing. Tone/noise decays lengthen and clicks soften; 808/606 also
reduce their second partial. 606 receives a −1 dB trim adjustment. Exact musical
values, normal/hard-velocity measurements and rejected hat trials remain in the
audit. Kicks, hats, claps, other lanes, sampled audio, filters and user-stored
sounds are unchanged.

Measured post-output 100 ms K-weighted body gains are roughly 0.84–2.17 dB,
with similar or lower peaks. These are host diagnostic measurements, not LUFS or
a hardware listening result. The initial hat boost was rejected because it
increased limiting and reduced groove RMS. Further hat/clap work remains separate.

## Target and verification

At the same .95 version, the allocator alone links at **584,240 B**, 48 B above
the 584,192 B baseline. The final normal **2.8.0.96** image, including the three
snare edits and version packing, is **584,256 / 584,336 B (80 B free)**.
Main static RAM is **97,580 / 98,304 B (724 B free)**, up 32 B; low RAM remains
28,928 B and pool 335,424 B. The factory dictionary grows from 242 to 244 rows
(34 raw table bytes before layout/alignment). No playback decoder or DSP is added.

`drum_allocation_test` covers free/matching/non-kick/all-kick priorities, actual
sampled and factory/SYN triggers, full six-voice pressure, retrigger, hat choke
and declick under UBSan. User-kit, drum editor, sound TUNE, studio recording,
hit actions, FX page and full 32-kit health/level checks pass.

The original 11,264-byte factory fixture is retained. Restored lookup checks all
509 untouched records byte for byte and applies a hand-reviewed field allowlist
for the three snares. The original noise-TUNE DSP fingerprint still renders the
frozen sounds; current factory/user TUNE parity is checked separately. The DSYN
bank fingerprint is explicitly updated for the reviewed three-record dataset.

Of 200 audio golden renders, only `drums/gm_kit` changes due to allocation under
pressure; its expected hash is updated. All other hashes are unchanged, with no
health, routing or CPU-budget failures. Both 14-scene × 13-mask render suites
pass with a delayed second core and with forced timeout. Target DSP/inlining,
LOFI hot-loop and exact startup restoration guards pass. Hardware audible and
deadline qualification remains to be performed.

Private link/test logs are in `build/drum-integration-96/`; the isolated measured
tuning candidate and same-level before/after WAVs are in
`build/drum-loudness-audit/tuning/`.
