# Drum loudness audit

## Recommendation

Bring in upstream 2.8.3's **drum voice allocator first**, then audition a focused
factory-kit balance pass. Do not start by boosting every drum or raising the
master limiter threshold. There is a real kick-truncation problem under dense
polyphony, but isolated kicks already have substantial peak level. Short,
transient-heavy snares and deliberately recessed hats/percussion are a separate
balance issue.

The audit below describes its frozen baseline and private experiments. The later
[allocator and three-snare integration](DRUM-PLAYBACK-INTEGRATION.md) records the
production changes, actual target link and validation separately. The audit
itself did not modify production sources, headers, shared outputs or installer.
At audit completion, the candidate had no target build or full integration suite.

## Scope and reproduction

Frozen working source identified itself as `2.8.0.95`; local `main` was upstream
commit `5eb24c1dc18261a85c193e8b28318944690e58fd`. Source, generated headers,
probes, binaries, CSVs and source hashes are isolated in
`build/drum-loudness-audit/`. `source/` retains the original source; the private
alternative is `drums-upstream-pick.c` and
`upstream-drum-pick-candidate.patch`.

- `hits.csv`: 1,184 isolated hits: 37 factory kits (five acoustic treatments and
  32 synthesized kits), 16 keyboard lanes, velocities 100 and 127, two seconds
  per hit. User sample slots and edited SYN kits are intentionally excluded.
- `grooves-current.csv` and `grooves-upstream-pick.csv`: 148 eight-second renders
  each: the same 37 kits, drum levels 100/127, simple/dense grooves at 120 BPM.
  Each has exactly 64 sixteenth steps. Simple is four-on-the-floor kick,
  backbeat snare and sixteenth closed hats. Dense adds clap, crash, eighth-note
  ride, sixteenth open hats and shaker. This is a deliberate six-voice pressure
  case, not a claim that every built-in groove has the problem.
- `synths.csv`: 171 factory presets, C4 at velocity 100, held one second then
  released for one second, factory trim and default synth level, dry sends and
  no insert/distortion. This is a controlled reference, not a complete musical
  preset-loudness calibration.

The probes call production drum/synth mixing and the production unity-master
quantization, DC blocker, look-ahead limiter and output knee. FX sends, master
LOFI/DUST and low-cut are bypassed. Each scenario starts with fresh relevant
voice/output state. Results are host DSP outputs, not recordings of the device's
speaker, headphones or analog output. `pre_k100`/`post_k100` use the same
K-weighting coefficients as `tools/level_drumkits.py`, taking the loudest 100 ms
of the mono center signal. They are diagnostic loudness proxies, **not integrated
LUFS**. RMS uses the entire stated render duration including silence/tails.

## 1. Dense grooves can prematurely lose their kick

Current `drum_synth_on()` and sampled `drum_on()` independently choose a free
voice or the oldest voice from six dedicated drum voices. Upstream's
`drum_pick()` instead uses a free voice, then restarts a voice already playing
the incoming sound, then steals the oldest non-kick before considering a kick.
The omission is already acknowledged in `UPSTREAM-2.8-INTEGRATION.md`.

The private experiment ports only that allocator into the frozen MerthSloop
source. Factory sound records, sample decoding, six-voice count, levels and DSP
are unchanged.

| Dense groove, level 100 | Active kick replacements, current → candidate | Output RMS dBFS, current → candidate |
|---|---:|---:|
| 808 | 16 → 0 | −15.77 → −14.53 |
| 909 | 16 → 0 | −17.88 → −17.81 |
| ACOUSTIC | 16 → 0 | −18.62 → −18.64 |
| AMBIENT | 16 → 0 | −16.24 → −14.98 |
| CHIP | 8 → 0 | −22.28 → −22.28 |

Across both levels and all kits the current scenarios replace active kick voices
936 times; the candidate does so zero times. Simple grooves have zero steals
and their measurements are unchanged. Preserving kick articulation does not
necessarily raise whole-mix RMS: compare ACOUSTIC/CHIP. The 808 improvement is
about 1.24 dB output RMS, plus preservation of its body. Allocation does not
explain quiet isolated hits or every kit's subjective balance.

## 2. Existing gain is not missing wholesale

`drums_mix()` uses `expr_gain(G_DRLVL * 200, TRK_DRUM)`. At default level 100
this is 20,000/32,768, approximately −4.29 dB; **the existing normalization tool
measures through this gain already**. Removing it as if accidental would undo
the measured calibration. Sampled hits additionally get ×1.75 (+4.86 dB), as
upstream does, to match synthesized kits. Both paths honor velocity and track
mute fades, center pan does not halve the signal, and sends add parallel wet
signal rather than subtracting dry. The bypass slicer does not apply level twice.

At velocity 100, none of the 592 isolated hits exceeds full scale before the
output stage. Synthesized kick median peak is −4.06 dBFS; its median K100 is
−10.38. The acoustic kick peaks at −2.28. At velocity 127, three isolated sounds
do exceed full scale before the output stage: ACOUSTIC RIM (+0.25 dB), 606 CLAP
(+0.85 dB) and BOOMBAP CLAP (+0.58 dB). The int32 mix and limiter accommodate
these; they are evidence against an indiscriminate boost, not reported output
clipping. Combined groove peaks already reach +8.17 dBFS before limiting.

## 3. Kit balance and transient-heavy sounds are credible weaknesses

| Velocity 100, level 100 | Pre-output peak dBFS | Pre-output K100 | Post-output K100 |
|---|---:|---:|---:|
| 808 kick | −5.18 | −10.39 | −10.68 |
| 808 snare | −4.45 | −22.01 | −22.71 |
| 808 closed hat | −4.77 | −28.63 | −29.06 |
| 808 open hat | −11.04 | −24.60 | −24.60 |
| 808 shaker | −15.72 | −29.87 | −29.87 |
| ACOUSTIC snare | −2.28 | −15.32 | −17.70 |
| ACOUSTIC closed hat | −12.64 | −24.19 | −24.19 |

The synthesized snare K100 ranges from −26.59 to −13.72 across kits at normal
velocity; peak leveling does not make their sustained body equally present.
The 808 snare reaches almost the peak ceiling while its 100 ms energy remains
far below the intended snare target. Merely turning up this transient makes the
master limiter work harder. A factory sound-design pass should inspect its
click/noise balance, envelope and body before increasing trim.

`tools/level_drumkits.py` intentionally targets closed hats 15 dB and shakers
17 dB below its base kick loudness target, with a further +2.5 dB kick target
offset. Non-kicks are constrained to −4 dB peak at the calibration velocity 110;
kicks to −1.5 dB. Normal playing uses velocity 100, about 0.83 dB lower in linear
velocity gain, with additional velocity-dependent filter changes on synth drums.
Ghost/soft/hard levels are 42/72/127; normal remains 100. The acoustic generator
likewise reduces hats by 9 dB, shaker by 8 and conga/cowbell by 4 before encoding.
These are intentional choices, but warrant musical review against the user's
preferred balance. Equal loudness for kick, hat and shaker is not the goal.

Dry C4 preset references have median K100 −11.57 before the output stage and
−14.10 afterward. Some synth bodies can therefore mask quiet drum lanes,
especially when multiple synth tracks/chords sustain. This single-note test is
not sufficient to assign a universal drums-to-synth correction.

## 4. More drum LEVEL has little output headroom

Drum LEVEL is linear 0..127, default 100: only +2.08 dB available above default.
Synth LEVEL is logarithmic, default −4 dB with a maximum +7.5 dB before preset
trim. The controls are materially different despite sharing the LVL label.

The production limiter threshold is 18,000 (about −5.20 dBFS), with roughly
35 ms hold and 46 ms release. A transient can turn down the body and following
hats as well as itself. In the simple-groove measurements, changing drum level
100→127 produces a median output RMS increase of only 0.43 dB. For 808 it is
0.54 dB simple / 0.40 dB dense. Raising the threshold or blanket gain does not
solve instrument masking or transient/body imbalance safely.

## Remaining path checks and proposed order

1. Integrate/target-measure the shared upstream allocator and add regression
   cases for same-sound retrigger, kick preservation, hats/choke, sample/synth
   kits and six simultaneous lanes. Keep six voices and declick behavior.
2. Audition a focused factory balance pass, especially short snares/claps and
   recessed hats/shakers. Prefer better body/transient envelopes and selective
   trims to a louder entire bus. Re-run all kits at normal/hard velocities,
   dense grooves, limiter reduction and representative synth mixes afterward.
3. Consider explicit per-lane gain or a drum body/punch control if wanted. A
   wider bus gain range could help mixing but will not circumvent the limiter;
   preserve the existing default and make the scale clear.

Runtime CC11 expression can attenuate the drum bus quadratically; CC7 changes
G_DRLVL. Track mute/solo, filter, insert, slicer and shortened per-lane DECAY
can reduce level/body when enabled. None is active in these measurements. Global
DUCK attenuates synths/texture on kick, not the drum bus. Global LOFI/filter and
the optional small-speaker low-cut affect the whole output; low-cut can reduce
kick weight and should be bypassed for headphone balance judgments. No always-on
drum-specific insert/filter/duck attenuation was found in the bypass path.

## Follow-up: restrained factory snare candidate

After authorization to integrate the allocator and tune individual kits, a
private three-sound candidate was measured against the same source with the
upstream allocator on both sides. It changes only the SNARE of 808, 606 and
VINTAGE. Per-kit copies avoid propagating a base-kit edit to its derived kits.
It adds no DSP and does not change pitch, filtering, velocity response, kicks,
hat choke, limiter, kit order or kit names.

| Kit | Data edits in musical generator units |
|---|---|
| 808 | Tone decay 120→180 ms; noise decay 170→250 ms; click 22→12; second partial level 92→76 |
| 606 | Tone decay 70→110 ms; noise decay 120→180 ms; click 24→14; second partial level 50→44; trim −1 dB |
| VINTAGE | Tone decay 60→85 ms; noise decay 95→130 ms; click 10→8 |

These envelope durations describe the generator's decay settings, not a
flat-level sustain of that length. The reduced click/partial keeps the body
extension from merely making a louder transient.

| Kit / velocity | Pre-output peak dBFS, original → candidate | Post-output K100, original → candidate |
|---|---:|---:|
| 808 / 100 | −4.45 → −4.93 | −22.71 → −20.74 |
| 808 / 127 | −0.86 → −1.56 | −24.06 → −21.89 |
| 606 / 100 | −4.00 → −4.09 | −22.51 → −21.67 |
| 606 / 127 | −2.06 → −2.15 | −22.19 → −21.13 |
| VINTAGE / 100 | −4.65 → −4.62 | −23.78 → −22.46 |
| VINTAGE / 127 | −2.13 → −2.10 | −24.14 → −22.83 |

All 1,184 hit measurements were rerun. Only six measurements (three sounds at
two velocities) change. Dense groove level-100 output RMS changes are 808
0.00 dB, 606 +0.06 dB and VINTAGE +0.00 dB; this is a more audible snare-body
proposal, not a large whole-kit RMS increase. The largest measured whole-groove
RMS reduction across both levels is 0.023 dB (VINTAGE dense at 127). Kicks and
allocator behavior remain unchanged. No new isolated pre-output full-scale
exceedances appear.

An initial hat gain/hold proposal was rejected: stronger isolated hats caused
more limiting and lowered simple-groove RMS by 0.35–0.60 dB. A subsequent
lower-click/lower-trim hat alternative improved isolated K100 with lower peaks,
but is deferred to keep this first integration within three sound records and
the target's very small flash budget. MINIMAL/GLITCH retain their deliberately
short characters. Claps are unchanged; some already exceed pre-output full
scale at hard velocity, so they need a separate body/transient investigation.

Reproducible artifacts are in `build/drum-loudness-audit/tuning/`:

- `snare-body-candidate.patch` changes only three generator entries and the 606
  snare trim. SHA256:
  `8338c81104c4724796db658de148b1df6d18f60062a48babb1d8c7f3da5121a8`.
- `encoded-baseline.bin` exactly equals the pre-existing 11,264-byte factory
  fixture. `encoded-candidate.bin` changes only factory `(kit,lane)` `(0,1)`,
  `(2,1)` and `(4,1)`. `encoded-allowlist.json` lists all 22 old/new bytes and
  changed field offsets; the other 509 records are byte-identical.
- `hits.csv`/`grooves.csv` are final candidate measurements. `v1-*`/`v2-*` retain
  the rejected/deferred hat experiments separately.
- `before.wav` and `after.wav` use identical levels/output processing and are
  48 seconds each: 808 simple/dense at 0/8 s, 606 at 16/24 s, VINTAGE at 32/40 s.
  They are rendered audition material; the audit does not claim a listening
  verdict from host statistics.
- `manifest.json` hashes the generator, levels, header, data, CSVs, patch and
  WAVs. `source/probe.c`, `source/groove.c` and `source/audition.c` are the
  private harnesses. Original private generated data and drum source were
  restored after export.

The factory dictionary grows from 242 to 244 rows, adding 34 raw startup-table
RAM bytes; compressed target flash delta must be measured by the integrating
build. No target size/deadline qualification is asserted for this candidate.
