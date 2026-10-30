# Master LOFI dropout investigation

Scope: frozen MerthSloop 2.8.0.92 working source in
`build/voice-lofi-investigation/baseline/source`. Production sources, firmware
packages, shared build outputs and Git refs were not modified by this audit.

## Findings

1. **Master LOFI does not intentionally remove instrument voices.** Its entry
   point is `vinyl_process_mode()` in `firmware/src/vinyl.c`. `mix_block()` calls
   it after the synth/drum render units have completed on both cores, their
   accumulators have joined, and the send effects have been added. Its writes
   are the private `vinyl` state and the supplied master stereo buffers. It has
   no allocation, note-off, gate, budget, recovery or instrument-parameter
   callback. Its RNG is private, separate from VOICE breath noise and the shared
   instrument RNG. The ring indices are masked to 0..255; SAMPLER clears bits
   of the already masked write index. No alias/array-bound defect was found.

2. **Every active character adds serial work on core 0 after the join.** That
   work is not parallel with the current block's instrument render. This makes
   insufficient instrument-render headroom a plausible explanation, but does
   not by itself prove a scheduling defect: a common postmix cost adds to every
   possible partition equally. The scheduling and overload-policy audits must
   supply the rest of the causal chain.

3. **Initial enable unnecessarily disables the existing steady-control path
   for about 93 ms.** `vinyl.bed` rises by 8 per frame from 0 to 32768, taking
   4096 frames, 128 control blocks or 16 audio halves. The single `steady` flag
   includes that noise-bed ramp as well as WOW/TONE/DAMAGE, wet blend and mode
   transition. Consequently all seven slew checks and coefficient calculations
   continue at sample rate while the bed alone is changing. With the tested
   controls, the other state settles after 16 blocks for VINYL, 17 for TAPE,
   VHS, CD and SAMPLER, and 24 for RADIO. OFF bypass does not advance `bed`, so
   the first enable during playback has this full startup path. A later enable
   with `bed` already full can have a shorter transient. This is avoidable
   work, **not proof that it crosses the FM-1 deadline**.

4. **Signal coloration alone cannot explain actual gate loss in these tests.**
   TAPE/VHS damage and CD intentionally disturb the master signal; RADIO cuts
   bass, and SAMPLER quantizes. VINYL has no note-selective gate. Filtering and
   delay can alter apparent chord balance, but no single such mechanism is
   common to every character. Do not infer that all audible thinning is merely
   spectral coloration, or that a host gate test proves target timing safety.

## Voice-state isolation evidence

Private `state_probe.c` runs each menu type in a fresh process, with OFF for
2048 blocks followed by the selected type through block 16000. Controls are
WOW 64, TONE 64, DAMAGE 40; voice budget is 16/24. Scenes are four VOICE notes
plus drums, three FLUTE DUST/GRAIN notes plus drums, and four LOFI-engine notes
plus drums. All expected notes remain active and gated after every block.

For each scene, **all seven menu types produce identical hashes of every
instrument voice-state byte and every engine-arena byte over all 16000 blocks**:

| Scene | Voice-state hash | Arena hash |
| --- | --- | --- |
| VOICE + drums | `9404521de09f3559` | `2a3bfab9f090a383` |
| FLUTE DUST + drums | `5517461459a9e1e5` | `d461238ce89200a5` |
| LOFI engine + drums | `80ac7cb452beb53b` | `2a3bfab9f090a383` |

Every run has zero allocation kills and zero shedding. This isolates LOFI's
intended routing; host `mix_block()` does not execute the hardware deadline
guard, so this is expected to miss overload-induced loss. Whole-mix RMS is
comparable across types; drums contribute to that measurement, so it is not a
per-note audibility test. Log: `build/voice-lofi-investigation/lofi/state-probe.log`.

## Private transition prototypes

Two exact-audio candidates were tried; neither is recommended as a demonstrated
performance fix. Both retain every existing effect, control range and slew.

**Separate bed slew:** remove bed from the all-controls steady predicate and
update it under a separate block-stable predicate. Frozen audio and full-state
hashes pass. The target image is 582356 B versus baseline 582324 B, a 32 B cost,
with unchanged static RAM. Cold-enable host runs improve, but some settled
characters become slower on the host because of the extra per-frame branch.
That tradeoff is not justified by target timing evidence.

**Separate coefficient steadiness:** retain the original seven slew checks,
but stop recomputing period/radio/band/low-pass coefficients every sample once
their inputs settle, independently of the bed ramp. The target image is
582340 B, a 16 B cost, with unchanged static RAM (main 97500 B, low 28928 B,
pool 335424 B). Exact source instrumentation over the first 128 blocks gives:

| Character | Baseline coefficient groups | Prototype groups |
| --- | ---: | ---: |
| VINYL | 4096 | 624 |
| TAPE, VHS, CD, SAMPLER | 4096 | 655 |
| RADIO | 4096 | 872 |

Stereo samples and complete private effect state match after **every block**
in this counter comparison. The maintained frozen `.88` output/full-state hash
test also passes across all modes, varying blocks, zero WOW, bypass, changing
controls, punch and transport, including a UBSan run. The target loop guard
still reports zero hardware divides and zero helper calls in sample loops.
The coefficient-group counts are source-level execution counts, **not target
instruction or cycle counts**. The target disassembly confirms a native build,
not a measured speedup. The existing nested-span estimator is unsuitable for
estimating the selected conditional path here.

Host benchmark results are mixed, including small regressions for the second
prototype. They are not evidence of faster target execution. No target deadline
improvement, dropout resolution or silicon timing qualification is claimed.

Private files are under `build/voice-lofi-investigation/lofi/`: `state_probe.c`,
`state-probe.log`, `transition_probe.c`, `transition.log`, `transition-coef.log`,
`coefficient_count.c`, `coefficient-count.log`, `candidate-target.log`,
`candidate-coef-target.log`, and `coefficient-only.patch`. The latter patch is
optional investigation material, not integrated production work.

## Recommended interpretation

The master effect is a load trigger and has a measurable startup fast-path
blackout. No direct voice-state mutation or effect-local corruption was found.
Prioritize the independently demonstrated worker-eligibility and irreversible
overload-shedding defects in the other specialist reports. Reassess transient
LOFI work after those corrections; do not ship these micro-optimizations on the
strength of host percentages or claim they resolve the reported hardware loss.
