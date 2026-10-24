# Audio review — current working tree

## Implementation follow-up — 2026-10-23

The authorized audio batch is implemented in the shared working tree, based on HEAD `270c0e3`. The original review below is retained as historical evidence; its review-only restrictions and “no production changes” statement describe the earlier review pass.

**AUDIO-1 fixed:** `punch.c` widens the two SPRING return/gain products, both FREEZE capture crossfades, the primary wet/dry crossfades, and the simultaneous filter wet/dry crossfades. Original operand preshifts, arithmetic right shifts and narrowing points are retained. Safe products therefore keep their old rounding/output, while products spanning opposite-polarity bus headroom no longer wrap. No clipping or audible-character change was introduced to conceal the error. `spring.c` and `fx.c` were not changed.

**AUDIO-2 fixed:** settled LOFI bypass clears `vinyl.bed` only while stopped. Playing bypass retains its previous behavior, and active STOP still slews the noise envelope. The stored TYPE/amounts are unchanged.

**Punch BSS optimization integrated:** the 188-byte punch and 36-byte punch-filter objects now use BSS. One explicit cold `punch_boot_init()` assigns the original nonzero defaults after hardware data initialization. `tests/hostsim.c` calls that same helper once at its first track bootstrap, rather than resetting effect state every time a test reloads track defaults. The helper is deliberately not a general reset function and assumes the normal zero-initialized cold state. Main startup's only edit is the helper call. The parent separately added the equivalent call at the start of `firmware/host/preview.c:preview_init()`; that file was not edited by this audio batch.

### New regression evidence

All evidence is private under `build/code-review-audio-51/`; no shared build outputs, package, version, Git ref, or runner registration was changed.

| Test | Before | After |
| --- | --- | --- |
| `tests/audio_freeze_range_test.c` | Fails: dry +4000000, return -196608, gain 1024 yields 8192000/-8192009 instead of -194560/194551. This specifically catches the crossfade failure even when the gain product itself is safe. | 41,472 stereo samples match a wide arithmetic oracle across both polarities, gains and dry headroom. Real SPRING capture/hold/release chains pass for both signs, each checking 55,232 return products outside int32 range. Also passes Zig signed-integer-overflow trap instrumentation. |
| `tests/audio_lofi_stop_test.c` | Fails: stopped VINYL re-enable emits absolute sum 180066 through the direct effect path. | Every character passes both zero-amount and OFF bypass transitions. PLAY resumes noise where the character generates noise; active STOP retains its fade. A separate saved-TYPE/events/real-mixer/master-chain sequence passes with at most the permitted two-LSB master DC tail. CD damages content and is not incorrectly required to generate noise on silence. |
| `tests/audio_punch_boot_test.c` | Original source used data initializers rather than this BSS/startup contract. | Verifies initial BSS, common nonzero defaults, untouched first dry block and that repeated host track setup does not reinitialize held effects. |

Existing `effects_plan`, `punch`, `reverb_types`, `vinyl`, `lofi_character`, `fx251` and `mix_split` suites pass against the isolated final audio source. Split checks cover seven scenes × eleven splits in CORES=1 and seven scenes × three splits in CORES=2, both with and without timeout injection. An instrumented private copy of the existing punch test produces **17 identical before/after full-render hashes**, covering safe filters/kills, ring effects, pan, freeze and fade renders (`integration-before-parity.log`, `integration-after-parity.log`). Production tests were not rewritten to accept new audio hashes.

The first post-change snapshot caught another chat's preset header mid-transition (ragged header with the old matrix consumer) and could not compile. It was discarded for measurement: paired snapshots use the same complete pre-change source, with only this batch's owned files overlaid for the after case. The later parent message confirms the shared trim header/consumer pair is now integrated. Also, an initially over-strict new test asserted a fixed noise loudness on all characters; it was corrected to check the actual lifecycle contract and not require synthetic noise from CD. Final logs contain the passing corrected tests. These were test/snapshot issues, not additional production edits.

### Private target measurement and limits

Both paired JieLi Clang 4.0.1 `-Oz` target builds succeed and pass `build.check()` and the existing target instruction-budget suite. Only the application was compiled/linked in private outputs; no firmware package was generated.

| Quantity | Before | After | Difference |
| --- | ---: | ---: | ---: |
| Application load bytes | 580,704 | 579,312 | **−1,392 B** |
| Main .data + .bss | 86,544 | 86,528 | −16 B |
| .data load payload | 524 | 300 | −224 B |
| Low RAM | 28,280 | 28,280 | 0 |
| Pool | 332,876 | 332,876 | 0 |
| mix_block symbol bytes (effects inlined here) | 15,616 | 15,620 | +4 B |
| mix_block disassembled instructions | 5,058 | 5,070 | +12 |
| mix_block loop calls / divides, static | 24 / 19 | 24 / 19 | unchanged |

This **combined** BSS/startup plus correctness delta is a paired measurement, not a promise to add the earlier 1,488-byte experiment to any independent savings. The before image already includes unrelated source work present when the snapshot was taken; parent must measure the final combined target independently. Link/global-layout effects account for savings beyond the 224-byte data payload, as in the original headroom audit. The cold initializer emits 40 target bytes.

`mix-block-target-cost.json` also records the target-budget parser's whole-function loop metric (42,472 → 41,929). Branch layout changes overlap its backward-branch spans; this is **not** a measured runtime speedup or per-FREEZE-cycle count. Widened arithmetic adds no new multiplication library symbol, loop call or divide in the paired target, but ordinary instruction scheduling and register pressure can still change. Actual hardware worst-case ISR duration and audio deadlines remain a required parent/hardware check. No hardware timing claim is made.

### Parent integration hooks

The preview startup hook is already supplied by the parent; no further production hook is required for this batch. Register the three new tests in `tests/run_tests.sh` from the parent-owned runner, using the ordinary host flags (`-O2 -UNDEBUG -Ibuild/gen -Ifirmware/src -Ifirmware/hal`, `-lm`). Consider retaining signed-overflow trap coverage for the range regression. The audio chat intentionally did not edit that runner. Parent still owns the combined target build, full integrated golden/UI/persistence checks and hardware deadline validation. No commit, push, flash or publication was performed.

## Original review

Reviewed 2026-10-23 on `merthsloop`, HEAD `9fd9ddc2f45d57ed0647270d757c2ead7ba30a0a`; the reviewed source is the working tree, including uncommitted changes, not HEAD alone. No production files, shared generated outputs, refs, or other reports were changed. Evidence and the source snapshot are exclusively in `build/code-review-audio-51/`. No repository AGENTS.md was found in the searched repository/ancestor locations.

## Integration recommendation

Fix the two confirmed P2 findings below in separate focused changes. Neither requires changing musical features, saved formats, factory content, partitions, or sample playback. Retain the current split architecture and DSP_INLINE policy. No new size/speed saving is measured by this review; do not book a flash saving from its structural opportunities. Existing OPTIMIZATION.md, ARCHITECTURAL-HEADROOM.md and REDUNDANCY-AUDIT.md describe already-integrated work and are not counted again here.

## Ranked confirmed findings

### 1. P2 — spring FREEZE return gain exceeds the signed product range

**References:** `firmware/src/punch.c:441` (both channel products), gain range at `punch.c:318`; spring pickup scale at `firmware/src/spring.c:59`.

The old FDN return fits the multiplication's assumed range, but SPRING returns `first*4 + second*2`: its positive peak can be 196602 from bounded int16 storage. FREEZE permits fg up to 16384. The expression `rev_l[i] * punch.fg` overflows signed int32 above a return of 131071 at that gain; even gain 11012 overflows at a return of 196602. This occurs before the protective master limiter, so that limiter cannot reconstruct the intended signal. The consequence is a wrapped/distorted held return, and undefined signed arithmetic can also affect compiler transformations.

**Verified reproduction:** `freeze_repro.c` drives the real spring bus, punch_freeze_feed and punch_process, not only a fabricated product. Prime the dry-level follower with 1000 blocks of +4000000 stereo input; select SPRING, hold FREEZE, and continue the hot input for 2000 blocks. The first violating block is 274: spring return 196602 on both channels, fg=11012, with 55232 sample positions having an out-of-range gain product. These values are a deliberate headroom stress input, not evidence of how frequently ordinary presets encounter the issue. The isolated `repro.c` additionally shows the arithmetic clearly: return 196602, fg=16384 yields output -1048161 through the current punch blend, while the widened return product is +3145632 before the blends.

**Why current tests miss it:** `tests/reverb_types_test.c:lifecycle` sets rev_frz directly and checks fx_buses, bypassing the later punch gain. `tests/effects_plan_test.c` exercises spring FREEZE with a low impulse and broad bounds, not this gain/return corner. Existing punch FREEZE tests primarily exercise the earlier FDN. Checking only an absolute ceiling can accept a wrapped value. All these existing suites pass in this review.

**Minimal candidate fix:** cast the return operand to int64 before multiplication, shift by 10, then narrow to int32; do this for both channels. Preserve the exact current rounding for every previously in-range product. In the same range audit, verify the two FREEZE capture blends at `punch.c:442-443` and outer wet blends at `punch.c:479-480`: their preshifted products also require the actual difference to stay within roughly 4.19 million. For a promised +/-4-million input contract and opposite-sign held material, use widened products while retaining the existing preshifts and rounding, rather than assuming the comment “no 32-bit overflow” proves the bound. Do not clamp the return merely to hide the product failure; that would change its audible character.

**Meaningful regression:** feed saturated spring history with both signs, sweep fg=1024..16384, and compare the FREEZE return and crossfades against a 64-bit reference that preserves the original shifts. Include the real capture/hold/release stress sequence and an opposite-polarity dry/held sequence. Require exact output equality on safe existing FDN/spring vectors. Recheck target flash, cycles and audio deadline because widening is in an active effect loop. No candidate patch or target cost was measured here.

### 2. P2 — bypass preserves a playing noise envelope across STOP

**References:** `firmware/src/vinyl.c:34-43` (early bypass), `vinyl.c:57` (only bed-envelope update), `vinyl.c:70` (noise application).

The bypass path resets the ring/filter/click state and returns before updating `vinyl.bed`. After LOFI has played, bed can remain 32768 indefinitely while bypassed. STOP during that bypass does not decay it. Enabling crackle later, while still stopped and with silent input, emits the remembered noise envelope for up to about 4096 samples (~93 ms). This contradicts the intended stopped synthetic-noise silence and is audible even though no transport or note was started.

**Verified trigger:** play silent input with crackle=127 for 200 CTL blocks; set crackle=0 and render 200 blocks to full bypass; stop and render another 1000 bypassed blocks; restore crackle=127 while stopped. `repro.log` records bed=32768 after the stopped bypass, then absolute output sum 88461 and peak 5761 over the next 200 blocks. No externally stored parameters or fabricated internal state are needed. The same envelope is shared by the other LOFI characters, though the measured values above use VINYL.

**Why current tests miss it:** `vinyl_test.c` and `lofi_character_test.c` stop while the effect remains active, giving the per-sample bed slew time to reach zero. Their later dry-bypass checks do not execute PLAY -> bypass -> STOP -> re-enable while stopped.

**Minimal candidate fix:** in the settled early-bypass path, clear bed when `!song.playing`. Clearing an inaudible envelope there avoids changing playing re-enable behavior or active STOP fades. Preserve the normal active path's 8-unit slew and the existing saved-parameter handling. A broader reset helper is unnecessary for this fix.

**Meaningful regression:** execute the exact public-control sequence above with zero input, verify exact zero throughout stopped re-enable (for each noise-generating character), then verify that PLAY still fades in noise and active STOP still fades it out. Include OFF-mode bypass and zero-amount bypass separately. This is an intentional correction to the stale-state transition, not a claim that every old transition output remains bit identical.

## Coverage map and clean areas

| Area | Inspected contracts and evidence |
| --- | --- |
| fx.c buses | Delay length clamp, power-of-two wrap, 64-sample tap transition, character-mode zero crossing, feedback/LP coefficients and int16 saturation; chorus interpolation; FDN partitions, variable active lengths, model fade/reset and FREEZE. Read delay_character_test.c and reverb_types_test.c. Delay feedback maximum 127*230=29210 is below unity; reverb tune maximum feedback remains below 32768. These bounds do not prove every nonlinear tail invariant. |
| fx.c mixing | Per-part preset swap scratch, gain ramps, mute/duck, level and pan, send bounds, insert/slicer/filter order, drum four-channel inserts and send filters, summation before shared effects, DAC/USB limiter paths. Inspected insert state transitions, swept buffers and filter coefficients. No additional confirmed defect in these inspected normal paths. |
| vinyl.c | Every character branch, history addressing and interpolation, mode fade, wow/tone/crackle slews, bypass and stopped-noise lifecycle. Found stale bed; did not identify an out-of-bounds history access. |
| punch.c | Hold rank/source/family ownership; independent primary/filter/gate/rate requests; ring writer and replay-span protection; loop/reverse/stop/rewind/flange/pan; freeze and simultaneous combination processing. Found spring gain overflow. Existing combinations preserve one primary ring write pass in the exercised tests. |
| slicer.c | Pattern/rate and swing clock, half-rate fractional remainder, segment boundaries, 2:1 capture capacity, stutter windows/interpolation, silent/busy distinction, drum sends. HALF/DOUBLE timing and full slicer tests pass. No newly confirmed buffer-wrap defect. |
| live modulation | voice.c live_mod_tick, pitch/tremolo/wah follower/pan; fx.c track routing and filter state; shared metadata read before dispatch and per-effect selected-part mutation. Read tests/modulation_test.c coverage via source search and mix_split_test.c live-mod scene. Did not independently rerun the UI modulation test. |
| audio.c / cpu1.c | CTL-sized ISR blocks, UAC tap before output scaling, limiter/output convention, overload deadline accounting, state transitions and dispatch/wait. Read HAL handoff barriers/hold and cpu1_host.c. Host tests do not validate hardware cache/barrier semantics. |

The core ownership design is coherent: separate acc/part scratch and preset scratch per core; track/insert/slicer/filter state owned by its render unit; shared effects and texture run after wait; RNG-sensitive LFO/PHYS and multiple FORMANT units are constrained by units_split. Fixed partition sizes and bounded active reverb indices are explicit. Current tests include actual impulse/response, energy, click, silence and split-output properties; they are substantially stronger than implementation-mirroring tests alone.

## Validation performed

All commands compiled the private working-tree snapshot, with existing generated headers copied into its private build/gen; no test runner that writes shared build/host or shared asset outputs was invoked.

- Zig 0.13 host compiler, -O2, assertions enabled: effects_plan_test PASS (HALF/DOUBLE transport and fractional handoff, outgoing MIDI clock, combinations, source ownership, SPRING lifecycle).
- vinyl_test PASS: frequency response, wow pitch excursion/stereo, PLAY/STOP noise, hot input and bypass.
- slicer_test PASS: click ramps, timing, sync, dry OFF and stutter playback; its cost counter reports unavailable on this host.
- punch_test PASS; fx251_test PASS; reverb_types_test PASS (all six models, damping/decay and lifecycle).
- mix_split_test PASS: seven scenes x eleven splits CORES=1; seven scenes x three splits CORES=2; same quick scenes with CORES=2 CPU1_TIMEOUT_AT=700 PASS.
- Custom repro.c and freeze_repro.c completed; exact outputs in repro.log and freeze_repro.log. Private scripts retain reproduction commands.
- Attempted signed-overflow instrumentation: Zig failed to link its UBSan runtime handlers; WSL has no system cc. No sanitizer-clean claim is made. Arithmetic and normal optimized reproductions establish the reported overflow independently of sanitizer availability.

## Measured optimizations

None proposed or measured. In particular, this review does not claim that merging the intentionally specialized hot loops saves bytes or cycles.

## Unmeasured opportunities and test gaps

1. Drum insert calls repeat settings-to-coefficient work four times per block for L/R/REV/DLY. A small immutable per-block coefficient preparation could potentially avoid redundant ins_lpk divisions and parameter decoding. This is an opportunity only: fade/type state remains per channel, output must be bit identical, and extra plumbing may cost more target code than it saves. Measure the actual JieLi -Oz link and active-loop cycles before adopting; do not add a generic per-sample abstraction or blanket noinline.
2. CPU1_TIMEOUT_AT is a lost-before-start job model. Hardware can time out after partial mutation of voice, filter, slicer or preset-swap state. fx.c:1282-1286 explicitly acknowledges the block glitch and re-renders from the mutated state; it is not transactional recovery. Existing timeout hash tests therefore prove the lost-job fallback, not deterministic recovery from an arbitrary halted instruction. This is a test/guarantee limit, not a newly reported defect: add controlled partial-unit failures if stronger post-timeout recovery is required, without introducing expensive whole-state snapshots into every normal audio block.
3. Broad boundedness checks should be supplemented with the 64-bit range oracle at return gain/crossfade boundaries. A wrapped negative or positive sample can remain under a broad absolute bound. Retain the spectral/behavioral tests, rather than replacing them with hashes of the current faulty arithmetic.

## Explicit limits

This is source review plus focused host testing, not proof of absence of bugs. No hardware run, target build/size delta, cycle measurement, ThreadSanitizer rerun, full golden corpus rerun or full UI/persistence suite was performed. Audio ISR/HAL timing, second-core halt atomicity, cache effects and maximum hardware stack use remain outside the host evidence. The overflow's full-chain stress uses deliberately hot synthetic input; its occurrence rate with factory presets was not established. Production input paths were not exhaustively searched for a tighter peak bound. No partition change, runtime sample decompression, musical feature removal or weakened validation is recommended.
