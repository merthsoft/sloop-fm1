# Audio review — current working tree

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
