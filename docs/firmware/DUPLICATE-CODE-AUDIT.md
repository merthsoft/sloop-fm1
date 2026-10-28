# Duplicate code audit: dirty .78 snapshot

2026-10-26. Two narrowly scoped candidates recover **560 full-image bytes together**, from **583,812 to 583,252 B**, leaving **1,084 B** in the real **584,336 B** application slot. Static RAM changes are **0 B** in every region. These are isolated review patches, not integrated firmware or demonstrated speed improvements.

## Frozen comparison

The audit copied the actual dirty `firmware/`, `tools/`, `tests/` and generated headers into `build/duplicate-audit-78/source/`. HEAD was `6017074eb43c4e1e6966fb934975a48dd88a3bf4` (.77); the frozen source identifies itself as **2.8.0.78** and includes the active snapshot/morph implementation and analog refactor. This is a same-version dirty-source comparison, not a comparison against committed .77 or the parent's subsequently changing .78 package.

Only this report and private files below `build/duplicate-audit-78/` were written. No production firmware, tests, tools, shared generated headers/build outputs, installer, version, budgets or Git state were changed. No loader, package, installer generation or flashing was run.

Private `run.py` invokes only `build_app()` and `check()`. Each variant uses its own complete source and output tree, the existing boot table/preset packing, SLICE=0, ordinary production flags and installed WSL toolchain `/home/shaun/.cache/sloop-host-tools/jieli-linux-toolchains-20250324.1` (Clang 4.0.1, `-target pi32v2`). Original source verification covers **326 files**, unchanged; interpreter caches are explicitly excluded. Generated-header hashes are retained separately. Per-result `commands.json`, symbols, sections, disassembly, image hash, placement checks and budgets retain the target evidence.

| Variant | Image B | Saved B | Slot free B | Disposition |
|---|---:|---:|---:|---|
| Frozen dirty .78 | 583,812 | — | 524 | Baseline |
| Dynamic coefficients, caller retains damping folding | 583,316 | **496** | 1,020 | Preferred coefficient candidate |
| Drum-hit priming reuses existing decoder | 583,748 | **64** | 588 | Small, low-risk candidate |
| Both preferred candidates | **583,252** | **560** | **1,084** | Measured combination |
| Dynamic coefficients, damping inside callee | 583,300 | 512 | 1,036 | Alternative; trades constant damping folding for 16 B |
| Previous alternative + drum priming | 583,220 | 592 | 1,116 | Tested, superseded by preferred combination |
| All coefficient setup outlined | 583,364 | 448 | 972 | Reject: loses fully constant setup folding |
| Force `drums_mix` noinline | 583,812 | 0 | 524 | Already shared by compiler |
| Force `mix_part` noinline | 583,812 | 0 | 524 | No additional sharing gained |
| Force `mix_drums` noinline | 583,844 | -32 | 492 | Reject: grows image |

Alternatives are not additive. The preferred pair happens to add exactly in this link, but that is measured, not assumed. Every row preserves main static **91,216/98,304 B**, low RAM **28,928/29,496 B**, and pool **331,852/344,064 B**. Main free is 7,088 B; low free 568 B; pool gap 12,212 B retains the existing 8,192-byte reserve. No new buffers, arena state, heap allocation or upstream format changes.

Baseline SHA-256: `79fe91a9cb463876f121345d66739520cf3be82074309086234ea3e470996c29`.
Preferred pair SHA-256: `504f1f111869fe6228e74fa68299baef43c5a0db07e37df1a78411c97e040a21`.
The final repeat is byte-identical.

## Ranked findings

### 1. Share dynamic SVF coefficient setup: 496 B

Patch: `build/duplicate-audit-78/coef-damping.patch`. Combined patch: `build/duplicate-audit-78/final.patch`; complete candidate: `final-source/`.

`tsvf_coef` is forcibly inlined into analog voice setup, track filter setup, texture WIND and punch filter setup. These are live compiler-specialized copies of the same coefficient algorithm. The patch retains the exact arithmetic in one inline damped kernel, plus an **82-byte** noinline dynamic wrapper. Fully constant cutoff/resonance calls still inline and fold; resonance-to-damping arithmetic stays in the caller so constant resonance still folds even when cutoff varies. A compile-time macro selects those paths, evaluates runtime arguments once, and casts resonance to the original int32 parameter type. Existing callers use scalar reads and addresses; no side-effect ordering contract is introduced.

This adds calls at coefficient setup boundaries, **not inside the audio sample kernels**. Coefficient rounding, clamp order, table interpolation and divisions remain exact. The helper writes only its supplied coefficient object and reads the immutable SVF table; it introduces no shared scratch, RNG access, ownership change or initialization dependency. Both cores can call it independently.

In the preferred combined image, `analog_body` shrinks 1,834→1,742 B, `djf_block_res` 284→202 B and `mix_block` 15,784→15,384 B while the new leaf costs 82 B. Together these changes account for 492 function bytes; final alignment/layout supplies the remaining four coefficient-candidate bytes. This saving does **not** recount sharing the analog LP/BP/HP sample loop.

Performance tradeoff: extra block/voice setup calls and register preservation versus less instruction footprint. Keeping damping in callers avoids an added resonance divide for constant-resonance sites. No hardware cycles, deadline margin, cache effect or stack high-water were measured. Prefer this bounded candidate over the 16-byte-smaller alternative unless hardware evidence favors the latter.

### 2. Reuse decoder when priming a drum hit: 64 B

Patch: `build/duplicate-audit-78/drum-prime.patch`.

Only the first `sample_next_inline(z,v,0)` in `drum_on` becomes `sample_next(z,v,0)`, reusing the existing melodic decoder. The **drum sample loop keeps its inlined decoder**. The compiler embeds note-on handling into `trk_note_on`; its body shrinks 1,832→1,766 B, resulting in 64 loaded image bytes after layout. This removes a cold duplicated ADPCM step without adding a decode call per rendered sample.

The existing helper has identical predictor/index/position updates with `loop=0`; it neither consumes RNG nor shares mutable decode state. Note-on gains one call and the generic helper's loop-policy test. This is a flash/clarity tradeoff, not a measured speedup. Factory/user sample bytes, predictor format, hit stealing/choke behavior and interpolation remain unchanged.

## Validation and limits

The frozen baseline, earlier combined alternative, and final preferred pair were tested privately. Final results:

- **200 unchanged golden renders**, zero health failures, voice/routing failures, CPU budget failures or crashes. Host timings are diagnostic, not target deadlines.
- **4,227,200 coefficient comparisons** against the original implementation: all resonance values 0..127, every cutoff -256..32768, complete coefficient structs identical.
- **18,360 decoder comparisons** across all 51 generated zones, both loop flags, both gate states and indices 0..89 (including the index cap); returned sample and complete voice state identical. Real user-kit tests additionally cover RAM-backed user slots, missing lanes, empty slots, choke and sequenced hits. Golden/sample-cache scenes cover continuing playback; the candidate does not alter the sample loop.
- `mix_split_test`: **8 scenes × 13 splits**, each under CORES=1, CORES=2, CORES=2/CHAOS=7 and lost-job timeout fallback. All output hashes match each mode's unsplit reference. Scenes include FORMANT shared RNG, PHYS/S&H ownership, sample cache, drum slicer/filter, presets changing while sounding, texture and drone load.
- `auto_split_test`, `engine_pitch_ownership_test`, `arena_test` with its required `ARENA_STATS`, `userkit_test`, `drum_noise_tune_test`, `slicer_test`, `startup_state_test`, `punch_test`, and the dirty `.78` `track_snapshot_test` pass. Baseline/final startup-state output is byte-identical. Snapshot checks include endpoints, locks, ownership and no MIDI/RNG side effects.
- All target budgets and **25 hot-function loop-call caps** pass unchanged. Placement checks retain 925 call-free RAM-code instructions, 60-byte core-1 loop, scratch containment, ROM-reference guards and all memory reserves.

The actual drum loop's static metrics are identical: 975 loop instructions, three divides, one loop call site, weighted estimate 3,630. Analog's sample loop retains zero calls. Its static span estimate falls 640→608 because setup work moves; that is **not** a speed measurement. `mix_block`'s broad spans change 22→24 call sites from block-rate setup; all callee work must be included in any cycle assessment. `final-costs.json` retains these diagnostics.

Initial harness runs omitted `ARENA_STATS` and the snapshot screenshot directory; both harness requirements were corrected in private outputs and reruns pass. No production fix was needed. Tests use pinned Zig 0.13 at -O2 with assertions enabled; hostsim strips target attributes, so behavioral checks do not validate target call cost/stack placement. No physical target execution or ThreadSanitizer run was performed. Before integration, repeat the same-version combined target build and hardware worst-case audio/stack checks, particularly maximum analog voices plus active track/texture/punch filters on both cores.

## Exclusions and reproduction

Read the existing redundancy, compiler flags, link, ambient DSP/ASM/synthesis and headroom reports before selecting candidates. Their adopted or rejected findings are not new savings here. A fresh relocation-aware exact-body inventory examines **938 emitted functions** and again finds only the previously documented 10-byte `dstep_mask`/`ota_rd16` and 22-byte wait/delay pairs. The five apparently unreferenced C sections are assembly entry points, not dead code. Mono/stereo drums already share one 3,116-byte `drums_mix`; both core dispatches use one `mix_units`. Reverb models and modulation effects already share their relevant infrastructure. Different texture/grain/sample decode policies are not flattened into per-sample generic calls. No blanket abstraction or global inlining policy is proposed. This bounded audit does not prove all semantic duplication exhausted.

From the repository, reproduce without using shared outputs:

```powershell
wsl -d Ubuntu --cd /mnt/c/code/sloop-fm1/build/duplicate-audit-78 --exec env FELUCCA_SLICE=0 python3 run.py baseline-repeat
wsl -d Ubuntu --cd /mnt/c/code/sloop-fm1/build/duplicate-audit-78 --exec env FELUCCA_SLICE=0 AUDIT_SOURCE=final-source python3 run.py final-repeat
wsl -d Ubuntu --cd /mnt/c/code/sloop-fm1/build/duplicate-audit-78 --exec env AUDIT_SOURCE=final-source AUDIT_NAME=final python3 host.py
```

The retained patches are review artifacts in ignored build storage; preserve them before cleaning that directory. Integrate neither automatically during .78 packaging nor by adding these savings to a different parent baseline.

## Synced DRIFT integration

The later .81 integration adopts the preferred coefficient-damping sharing patch and drum onset priming through sample_next; the drum sample loop remains inline. The combined feature build is 584,332 B with 4 B free and no RAM growth over .80. Historical isolated savings above are not additive to this different baseline. The coefficient/priming contract test compares original formulas and decoder state exhaustively.
