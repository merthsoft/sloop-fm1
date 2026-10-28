# SAMPLE, GRAIN and drums performance audit

2026-10-27; read-only audit of the dirty `merthsloop` .89 working tree. Experiments and patches are private under `build/engine-opt-samples/`. No production source, shared generated output, toolchain, Git ref, package or served asset was changed. Master `vinyl.c` was frozen as an input and was not optimized here.

## Recommendation

Start with **lazy GRAIN density calculation**. The copied Flute Dust/chords/drums/master-LOFI scene evaluates its interval **48 times instead of 4,500**, with identical output, zero linked flash growth and zero RAM growth. This removes 4,452 calls to a target helper containing three hardware divisions. It does not change how often grains start, nor their RNG draws.

Consider **aligned reverse windows** next: the same scene decodes **212,944 instead of 222,172** GRAIN samples, with identical audio. It costs **16 image bytes** and no RAM. This helps occasional reverse-grain peaks; it does not make ordinary forward starts cheaper.

Two additional independent exact candidates are available: specialize the known pitch-table range to remove one division per `pow2_q16()` call, and reuse GRAIN's already proven exact IMA magnitude in SAMPLE/sampled drums. Neither produces additional linked flash headroom in this snapshot. None is device timing qualification or proof that .87's reported 6,333-us peak will fall below the approximately 5,805-us deadline.

## Frozen inputs and complete links

`snapshot/` was copied from the actual dirty firmware, tests and tools, together with existing generated headers, before experiments. `HEAD.txt`, `frozen-dirty.patch`, `grain-original.c`, `sample-original.c` and `frozen-firmware-sha256.json` retain provenance. The manifest describes frozen firmware, substituting `grain-original.c` for the subsequently edited private GRAIN file. This is not a clean-HEAD export or a claim that the parent's live tree stopped changing.

No applicable `AGENTS.md` was found at repository/ancestor or relevant firmware/docs directories. Read the requested optimization, duplicate-code, headroom and voice-budget reports plus ambient synthesis/ASM, architectural and sustain audits. Existing .88 GRAIN magnitude correction and 448 midpoint checkpoints, native clamp, shared drum onset decoder, shared analog/SVF work, boot packing and sample scratch reuse are baseline work, not new savings.

Pinned target: WSL Ubuntu JieLi Clang 4.0.1, `jieli-linux-toolchains-20250324.1`, normal `-Oz` flags, single `felucca.c` compilation unit, ordinary two-pass presets/literal pooling, `FELUCCA_SLICE=0`, normal About configuration (no diagnostic override), unchanged version `2.8.0.89`. Private `build_app()`/`check()` only; no loader/package generation. Existing SDK path was supplied by the task; this application-only link did not need SDK regeneration. Host validation uses installed Zig 0.13.

| Independent variant | Linked image B | Free / 584,336 B | Main static B | Low RAM B | Pool B |
| --- | ---: | ---: | ---: | ---: | ---: |
| Frozen normal baseline | 584,292 | 44 | 97,324 | 28,928 | 335,424 |
| Lazy GRAIN density | 584,292 | 44 | 97,324 | 28,928 | 335,424 |
| Aligned reverse windows, boundary-safe | 584,308 | 28 | 97,324 | 28,928 | 335,424 |
| Shared exact SAMPLE/drum IMA magnitude | 584,292 | 44 | 97,324 | 28,928 | 335,424 |
| Exact pitch-table range specialization | 584,292 | 44 | 97,324 | 28,928 | 335,424 |

These are complete same-version target links, not function-size sums. No combination was linked; do not add independent savings or assume their final placement. Main free space is **980 B**, low RAM **568 B**, pool gap **8,640 B**, only **448 B above the retained 8,192-byte reserve**. These are static extents, not stack high-water measurements. In particular, older diagnostic-build or .88 figures are not this normal-build budget.

Each named directory retains ELF, object, binary, disassembly, symbols and `result.json`. Baseline SHA-256 is `0e80c361f982186a11b3351d321277c01b71a130334991a2703122a212c5937e`. Lazy density: `0fa18b5a4a4bd789ab975ae0815d8d4d4ea520dc6982ebdcf61847889d38f0b9`. Safe reverse: `18456c43ab72a183586ee8e25fba33ff8e1857180c249d56cc4b8e971ced2b4b`. SAMPLE IMA: `d4b8f71dacd23dc00e05ff11612e8a8b69aa88b16be9ffa378918771062e171e`. Pitch specialization: `86c9ab7ac26dd3642f752903b7ec903081eadae3672bf10d55ca777185c68dad`.

## Ranked candidates and boundaries

### 1. Calculate the density interval only when due

Source: `firmware/src/eng_grain.c:404–418`; patch `build/engine-opt-samples/density-lazy.patch`.

Move only `iv = (44100u * pow2_q16(-p[P_E3] * 10)) >> 16` inside the existing `v->s[1] <= 0` branch, ahead of its current RNG draw. `iv` is used only there and by its `gr_spawn()` call. `pow2_q16()` reads the restored immutable pitch table and has no state/RNG side effects. Countdown subtraction, jitter truncation, parameter reads on actual spawn blocks, readiness/load rejection, voice limits and RNG consumption/order remain unchanged. Parameter changes while counting down still affect the next scheduled start exactly as before; there is no stale parameter cache.

The actual target baseline calls `pow2_q16` before countdown subtraction at `grain_render+0x350`. Candidate assembly checks the countdown and jumps past that call when positive. The helper contains **three hardware divides**, as shown in the retained `*-density-call.txt` snippets and full disassembly. In the copied 1,500-block `flute_lofi` scene, instrumented interval evaluations are **4,500 → 48**, decode count stays **222,172**, and audio FNV-1a stays **8313984cd54594c9**. This is an operation count, not hardware cycles. The sample-loop estimator remains 1,693 because the removed work is outside its counted loops.

This is the smallest integration patch and the clearest maintenance improvement: compute the interval at the only point that needs it. It lowers recurring block work, not the cost of a block on which a grain really starts.

### 2. Align reverse decode windows, retaining the interpolation boundary fallback

Source: `firmware/src/eng_grain.c:209–217`, spawn initialization `:284–290`, reverse refill `:333–345`; patch `build/engine-opt-samples/reverse-aligned.patch`.

Current `gr_fill()` seeks to `hi-127` and then decodes through `hi`. Successive windows retain the initial arbitrary offset, repeatedly decoding a prefix just to recover predictor state. For non-boundary `hi`, use `lo = hi & ~(GR_RB-1)` instead. This aligns with the existing full/half-segment checkpoints and decodes only the samples needed in that initial partial window. Subsequent windows usually become full aligned windows. No extra PCM cache, checkpoint, state field or source decoding layer is added.

**The `hi % 128 == 0` fallback is essential.** Spawn asks for `hi=pos+1` and then reads both `pos` and `pos+1`; blindly aligning that `hi` drops the former sample and causes an invalid buffer index. The delivered patch retains the original low-bound formula for this case, including `hi=0`. Do not integrate the earlier `aligned/` artifact: it deliberately predates this correction and is invalid despite fitting flash.

The new `rlo` and unused buffer contents differ internally; output samples, position/fraction, interpolation samples, window progression, grain lifetime and ownership match. Decoder replay is deterministic and consumes no RNG, so skipping unneeded replay preserves all future played samples. The seek starts no later than the previously indexed `hi`, uses the same readiness checks, and retains the existing incomplete-midpoint fallback.

Final normal/UBSan comparison covers **328,458 reverse grains and 44,422,594 render calls** across all built-in sources, cold/full indexes with dirty storage reused across sources, every 0..255 offset, near-source-start/middle/final indexed segment, six rates 256..131,072 Q16, and varying 1..32-sample render sizes. Every output and playback-state comparison passes. Including seek replay, total decode calls are **374,695,944 → 269,349,160**; maximum observed spawn/render-call decode count is **383 → 257**. These are the harness's observed maxima, not a formal global wall-time bound. See `verify.py`, generated private `tests/reverse_audit_test.c`, and `reverse-ubsan-cold.log`.

The real copied Flute Dust scene preserves hash **8313984cd54594c9** and reduces decode calls **222,172 → 212,944**, about **4.15% of GRAIN decodes**, not 4.15% of total CPU. Flute Dust RAND is only 10, so reverse grains are uncommon. Benefit depends on random reverse starts and refill alignment. Target `gr_fill` grows from 212 to 228 function bytes; complete image growth is independently measured as 16 B. Before integration, update the old full-window comment and explain the two-sample boundary fallback rather than leaving the dense ternary unexplained.

### 3. Specialize the pitch-table range without changing its values

Source: `firmware/src/eng_sample.c:31–36`, generated `pitch_inc()` contract in `tools/gen_tables.py:75–83`; patch `build/engine-opt-samples/sample-ratio.patch`.

`pow2_q16()` calls `pitch_inc(1600 + u%192)`, so its pitch argument is always 1600..1791. The generated top-octave lookup therefore shifts by exactly two for remainders 0..63, and one for 64..191. Selecting those two ranges directly removes the division inside `pitch_inc`, retaining the original rounding and the existing runtime table-derived denominator. No hard-coded frequency or rounded replacement ratio is introduced.

All **9,216 inputs -3072..6143**, the full documented-lower-bound domain for which the original shifts remain defined, match the original helper exactly. Defined engine controls use a subset. This does not claim to repair pre-existing undefined shifts for malformed/extreme inputs outside that range. Actual target `pow2_q16` has **two divides instead of three**, saves one callee-saved register in its prologue, and shrinks 96 → 86 function bytes; final linked image remains identical in size because of layout/alignment. No stack high-water test or cycle improvement is claimed.

This is lower priority than the lazy-density edit: it specializes a generated-table layout and needs a comment deriving 64/128 and a maintained exhaustive oracle if adopted. It benefits remaining GRAIN startup/size/rate calculations and SAMPLE/drum pitch setup; its density-call benefit interacts with candidate 1 and must not be added naively.

### 4. Apply the existing exact IMA magnitude to SAMPLE and sampled drums

Source: `firmware/src/eng_sample.c:124–144`, sampled drum hot loop `firmware/src/drums.c:426–437`; patch `build/engine-opt-samples/sample-ima.patch`.

Move the proven magnitude expression into an inline `ima_delta()` beside the shared IMA tables, retain `gr_delta()` as an inline compatibility wrapper, and replace SAMPLE's three conditional additions with it. Keep SAMPLE's index-88 cap, both predictor/index clamps, held-loop check, state reset/return order, interpolation, and the existing inlined drum decoder. This is reuse of .88 arithmetic in two additional consumers, not another claim for the already integrated GRAIN change.

All **524,288 step/nibble magnitude cases** pass. A separate frozen-original decoder oracle passes **275,400 complete-voice-state comparisons** across all 51 zones, start/loop/end positions, both loop/gate flags, index 0..89 (including the cap), and predictor extremes/zero. Existing user-kit tests pass. The whole target image and RAM are unchanged. Static drum loop instructions fall **975 → 969**, weighted cost **3,630 → 3,582**; `sample_next` falls 69 → 68 emitted instructions. GRAIN's loop estimator remains unchanged. These are static estimates, not measured target speed.

It cannot directly accelerate synthesized 808/other synth drum kernels or forward GRAIN decoding, which already uses this magnitude. Preserve the explanatory correction comment beside the shared helper when integrating; the private minimal patch leaves that comment with its compatibility wrapper.

## Verification and reproduction

- Each of the four independent candidates passes **200 unchanged golden renders**, zero health/routing failures and zero crashes. Host timed CPU notes are noisy and are not offered as device-cycle evidence.
- Lazy density and safe reverse independently pass existing `grain_seek_test`, `userkit_test`, `auto_split_test`, and **11 scenes × 13 masks** under one core, two cores with `CHAOS=7`, and lost-worker `CPU1_TIMEOUT_AT=7`. This includes Flute Dust/master LOFI, sample sharing, engine/preset transitions and real mixer fallback.
- SAMPLE IMA additionally passes the full **11 × 13** chaotic two-core split matrix. Pitch specialization passes **11 × 3** quick chaotic splits, user kits, its exhaustive arithmetic oracle and goldens. It has no new shared mutable state; no separate pitch-candidate timeout matrix is claimed.
- All five target images (baseline plus four candidates) pass copied target budget/inlining checks and standard placement checks. RAM flash driver stays 925 instructions without calls, core-1 loop 60 B. `metrics.json` retains per-function diagnostics; it does not account for dynamic checkpoint hits or prove speed.
- Final cold/built reverse comparison passes UBSan with `-fsanitize=undefined -fno-sanitize-recover=all`; no sanitizer suppression was needed.
- A separate 1,500-block real-mixer run switches sounding GRAIN from built-in flute to a simulated user slot, replaces its zone and increments `smp_user_gen`, empties that slot with another generation change, then returns to built-in flute. Baseline/lazy-density/safe-reverse audio hashes all match **f6e48f0462161b93**. Density evaluations are **2,754 → 33** for the lazy candidate; reverse decodes are **221,141 → 216,721**. This validates the GRAIN reset/readiness path with changed live zone metadata, not physical concurrent flash programming. See `workload-source-*.log` and `workload.py source`.

Private drivers are `run.py baseline|density-lazy|aligned-safe`, `sample.py [sample-ima|sample-ratio]`, `tests.py [label]`, `sample-tests.py`, `ratio-tests.py`, `verify.py`, `workload.py`, `metrics.py` and `checks.py`. Run target/host drivers from WSL using their absolute private paths. They write only below this audit directory. `run.py` reselects its private GRAIN variant; `sample.py` reselects its private SAMPLE variant. Select the matching variant before rerunning its tests, and do not rerun source freezing against the parent's changing tree. Some logs from an initially interrupted, unnecessarily slow chaotic golden run were replaced by the completed one-core golden run; dedicated chaotic split logs are retained separately.

## Rejected/deferred work and integration risks

- **Unconditional aligned reverse windows:** rejected due to the interpolation-pair boundary bug above. The size-neutral `aligned/` binary is not a handoff candidate.
- **Skip inaudible grains, suppress failed-spawn randomness, reduce reverse probability/density, stagger starts or spread index builds over different blocks:** these change RNG order, output, startup readiness or timing semantics. They are optional sound-changing policies, not exact optimizations, and are not recommended for this integration.
- **Decode SAMPLE/GRAIN through one generic per-sample function:** reject as a maintenance abstraction here. SAMPLE carries loop/reset/held-tail behavior and a defensive index cap; GRAIN seeks/replays deterministic state and has no per-sample voice-loop policy. Keep only the tiny exact arithmetic shared and inline. Existing drum onset outlining already saves cold duplicate code and is not new work.
- **Cache all parameters or add PCM/source caches:** unnecessary for the lazy calculation and too expensive to propose from historical RAM figures. The current normal pool is only 448 B above reserve; checkpoint storage already occupies the existing PHYS-sized engine union. Additional arena state requires target and 64-bit host layout assertions, source/upload generation invalidation and engine-adoption tests. Flute's 246 entries already fit the existing 448 midpoint prefix; merely extending that prefix will not help this reported source.
- **Split the 256-decode index build into two loops:** potentially removes the per-iteration midpoint-store condition during cold startup, but may duplicate the inline decoder under this compiler. Unmeasured; not a size or cycle claim. It cannot explain sustained overload once all 246 flute entries are ready.
- **Hoist drum mute/gain ramps or fuse multiplies:** `drums_mix` repeats the attenuation expression across voices, but its sampled gains differ by velocity and its nested Q15 products have observable truncation. Reassociating products is not exact. A settled-control specialization might help; it is not measured here and must preserve sample/synth ordering, `ds_control()` timing, declick tails and mute ramps.
- **Skip drum noise/oscillator work when envelope values look quiet:** `ds_render` consumes per-voice RNG for source and click paths, interpolates click/noise levels through a block, and advances tone/metal phases before `ds_control()`. Removing apparently inaudible work can change later noise/phase or shared observability. No safe broad skip was established. The existing small unrolled six-metal-oscillator kernel is not a reason for an assembly rewrite.
- No runtime factory-sample decompression, sample shortening, quality reduction, budget increase, voice ownership change, scheduler change or overload-guard change is proposed.

Integrate candidate 1 first, then independently decide whether 16 bytes for candidate 2 is worthwhile in the parent's final .89 link. Candidates 3/4 are small exact follow-ups with explicit maintenance tradeoffs. Rebuild the chosen combination against the final master-LOFI implementation and repeat the relevant audio/split/source tests. Device acceptance still requires the reported drums/Flute Dust chords, master LOFI types, same-boot SHEDS/LATE/MAX-US deltas and listening. Keep the expanded 16/24 admission setting and 95% two-half guard unchanged.


## Parent integration follow-up

All four measured candidates are integrated in .90. The actual combined normal link is
584,228 B (108 B free), with unchanged static RAM. DRAWBAR caching fits when
combined with the other selected changes. Full combined validation and the
deferred findings are recorded in [the integration report](ENGINE-PERFORMANCE-INTEGRATION.md).
The original audit measurements above remain independent baseline evidence.
