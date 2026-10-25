# Sustain flash-headroom audit

Audit date: 2026-10-25. Scope: same-version MerthSloop .67 dirty-source snapshot on `merthsloop`. No production firmware/tool/test source, shared generated header, shared target output, version, package, served file, commit or Git ref was changed. The report is the only non-ignored output. All experiments are under `build/sustain-headroom/`.

**The follow-up below supersedes this initial recommendation:** a no-additional-RAM combination saves **464 B**; a larger startup-string combination saves **2,276 B**, with a substantial RAM trade and hardware boot qualification still outstanding.

**Initial finding: the bounded minimum-cost build-time LZ4 parser saves 112 B.** It changes only `tools/lz4blk.py`, preserves the installed decoder/block format, and recovers **112 actual linked application bytes** beyond the already-integrated .66 compressor improvement. It consumes no additional target RAM and introduces no audio-path work. This is a small measured gain, not a new multi-KiB reserve.

## Actual complete links

| Frozen same-source/version variant | Application B | Free B | Main static RAM B | Low RAM B | Pool B |
| --- | ---: | ---: | ---: | ---: | ---: |
| Baseline, copied generated inputs | 582,964 | 1,372 | 89,456 | 28,928 | 331,852 |
| Baseline, all assets regenerated with original compressor | 582,964 | 1,372 | 89,456 | 28,928 | 331,852 |
| Minimum-cost parser | **582,852** | **1,484** | **89,456** | **28,928** | **331,852** |
| Final bounded parser, repeated complete build | **582,852** | **1,484** | **89,456** | **28,928** | **331,852** |

Slot remains 584,336 B. Main/low/pool capacities remain 98,304 / 29,496 / 344,064 B; the pool reserve remains 8,192 B. All link placement/check results pass. `.text` shrinks 112 B; `.data`, `.bss`, `.ram_text`, `.c1_text`, `.lowram`, `.pool` and `.noinit` sizes remain unchanged. These are complete image deltas, not summed function estimates.

Baseline and regenerated-control SHA-256: `bc1540deb717d005fc990cc0dc5f3fdf53fa766e612376555344b13494d7efbd`.
Candidate and repeated-final SHA-256: `ad04a465507328a4a9ab6d07e87b7821bccf193025822a89b35141d825361141`.

Evidence links: [baseline](../../build/sustain-headroom/results/baseline/result.json), [regenerated control](../../build/sustain-headroom/results/baseline-regenerated/result.json), [candidate](../../build/sustain-headroom/results/optimal/result.json), [repeat](../../build/sustain-headroom/results/optimal-repeat/result.json). Each directory retains actual ELF, binary, object, disassembly, sections and symbols. Generated control/candidate headers are retained in their `gen/` directories.

## Patch and mechanism

Apply [optimal-lz4.patch](../../build/sustain-headroom/optimal-lz4.patch) to the current `tools/lz4blk.py`. The complete final candidate file is [lz4blk-candidate.py](../../build/sustain-headroom/lz4blk-candidate.py).

The existing lazy parser chooses a locally long match. The candidate retains that implementation as `_compress_lazy`, then computes a minimum-byte parse for inputs up to 8,192 B. A backward dynamic program includes literal-token and match-length extension costs and compares every legal literal run and match length. Offsets have the same two-byte encoded cost, so a longest available match at each position represents all shorter legal lengths. It preserves the existing strict final-match-start and five-literal-tail rules, 65,535-byte window and overlapping-copy behavior. If the old lazy/fast result is smaller, it is retained. Larger inputs use the existing parser unchanged.

The bound limits quadratic Python-side work and does not alter any target capacity. Full Windows generation took approximately 12 seconds versus approximately 1 second for the warmed old parser on this machine. This extra offline build cost is the principal tradeoff. No device-side encoder, new compression mode, runtime sample decompression, destination allocation or startup call is added.

The 22 boot-table payloads fall from 7,338 to 7,253 B (85 gross payload bytes); final preset/zone packing and target alignment produce the **112 B complete linked result**. Do not sum 85 with another estimated saving. UAC FIR and predictor/bit decoding remain unchanged.

## Validation

- Existing LZ4 contract suite passes: deterministic output, round trips, fast fallback, offsets, overlapping copies, tail legality, window edge and extension boundaries; small/random and repetitive inputs included. Inputs above 8 KiB exercise the existing fallback.
- Existing C boot-table suite restores **22,032 original bytes** and passes malformed-input bounds checks.
- Existing raw and boot-enabled preset tests produce byte-identical canonical output for **171 complete factory records and names**; repeated restoration and scratch guards pass.
- Existing raw and boot-enabled startup-state tests produce byte-identical state/zone output and preserve live-state behavior on later track initialization.
- Target budget and target inlining suites pass. RAM flash-driver code remains 925 instructions without calls; core-1 loop remains 60 B.
- Complete linked **303,054 B `SMP_DATA`** is byte-identical, SHA-256 `dcad9fe8f8c08a796d1b433c5cc0f96af46416502833b256593994fe7cccfe17`.
- Regenerated baseline and candidate table, FIR and preset byte-oracle files are identical. Frozen private firmware, tests and assets retain every manifest hash. See [evidence.json](../../build/sustain-headroom/evidence.json) and [validation logs](../../build/sustain-headroom/validation/).

No full audio-golden or hardware cold/recovery boot, timing, stack or deadline run is claimed. This patch changes only offline lossless representation; existing firmware source and restored values are unchanged. Rebuild and validate the parent's final sustain combination rather than assuming an invariant 112 B across later source/layout changes.

## Reproduction and provenance

`freeze.py` copied actual tracked and untracked dirty firmware/tools/tests/assets plus current `build/gen`; it did not export HEAD. [manifest.json](../../build/sustain-headroom/manifest.json) records 568 input hashes. The private original tool tree is retained. The baseline exactly reproduces the supplied .67 figures.

Target compilation uses installed WSL Ubuntu JieLi `/home/shaun/.cache/sloop-host-tools/jieli-linux-toolchains-20250324.1`, ordinary `build_app()` and `build.check()`, `-Oz`, existing boot flags, unchanged version and `FELUCCA_SLICE=0`. Only application compile/link/check is called. Native restoration tests use installed Zig 0.13.0. Windows Python supplies Pillow for generation; WSL system Python lacks Pillow, so generation there was abandoned before any target measurement. The regenerated-control check proves that final Windows generation reproduces the original inputs semantically and byte-for-byte in the resulting baseline image.

From repository cwd in PowerShell, the retained final private tree can be rebuilt with:

```powershell
python build/sustain-headroom/generate.py
wsl -d Ubuntu --cd /mnt/c/code/sloop-fm1/build/sustain-headroom --exec python3 run.py optimal-repeat
wsl -d Ubuntu --cd /mnt/c/code/sloop-fm1/build/sustain-headroom --exec python3 validate.py
python build/sustain-headroom/evidence.py
```

Do not rerun `freeze.py` against the existing audit directory: it would replace the frozen inputs with the parent's newer source. `run.py` never packages or invokes shared output paths.

## Prior work and rejected claims

Read the earlier link/assets/storage audits and ambient synthesis before selecting this experiment. Fixed font metrics, ragged preset trims, boot numeric tables, zone/name packing, zero sentinel packing, shared chord walking and dead TRACKS dispatch are already integrated and are not credited again. The .66 lazy/hash-chain compressor's 768 B recovery is already in the baseline. Previous cold-state conversion regressed the current integrated layout and is not recommended. Shared analog rendering/assembly alter hot paths and remain outside this audit's no-audio-change constraint. GC, merged-string counts, symbol stripping and raw unlinked compression ratios are not additional demonstrated flash gains.

No other unmeasured candidate is offered as headroom. The delivered patch is independent of the parent's voice/envelope/engine, parameter and UI sustain edits and needs only the ordinary asset-generation path for integration.


## Follow-up: larger measured candidates

The follow-up retained the original .67 dirty-source snapshot. It did not recapture the parent's developing sustain code, edit production files, or change any shared build output. All numbers below are complete actual target links against **582,964 B**, not added independent estimates.

| Candidate | Application B | Recovered B | Free flash B | Main static RAM B | Pool B |
| --- | ---: | ---: | ---: | ---: | ---: |
| Existing baseline | 582,964 | 0 | 1,372 | 89,456 | 331,852 |
| Cold persistence/control sharing alone | 582,612 | **352** | 1,724 | 89,456 | 331,852 |
| Cold sharing + minimum-cost LZ4 | **582,500** | **464** | **1,836** | **89,456** | **331,852** |
| Pooled startup strings, original lazy compressor, no persistence sharing | 581,256 | **1,708** | 3,080 | 95,952 | 334,772 |
| Actual complete combination: cold sharing + minimum-cost LZ4 + pooled strings | **580,688** | **2,276** | **3,648** | **95,944** | **334,784** |
| Complete combination, second full build | **580,688** | **2,276** | **3,648** | **95,944** | **334,784** |

Low RAM remains **28,928 B** in every row. The complete combination leaves **2,360 B** main static margin and **9,280 B** gross pool margin, including the untouched **8,192 B reserve**; only **1,088 B** remains above that reserve. Main/pool usage grows **6,488 / 2,932 B**, respectively. This is a real RAM-for-flash trade, not free storage or a measured stack/deadline margin. The no-additional-RAM 464 B combination is the smaller integration choice if those RAM margins need to remain available.

The complete combination's SHA-256 repeats exactly: `5c131e18cacc00db515883a59fb510658688c6c7e12cbbc3c17928aa8da18b14`.
No-additional-RAM combination SHA-256: `ca15bd04ea58cdb527b8722def5b8065a46d183db6e66f49c71ed59a2f13280e`.

Actual link records: [cold sharing](../../build/sustain-headroom/persistence/results/all-sharing/result.json), [no-additional-RAM combination](../../build/sustain-headroom/persistence/results/sharing-plus-lz4/result.json), [pooled strings](../../build/sustain-headroom/strings/results/pooled-strings/result.json), [complete combination](../../build/sustain-headroom/combined/results/combined/result.json), [repeat](../../build/sustain-headroom/combined/results/combined-repeat/result.json). Every result retains ELF/bin/object/disassembly/sections/symbols. The complete target's `.text` is **577,324 B**, `.data` 348 B, `.ram_text` 2,956 B and `.c1_text` 60 B: these sum to 580,688 B. No linker region was enlarged.

### Cold persistence and editor sharing

[Cold-sharing patch](../../build/sustain-headroom/cold-sharing.patch), modifying only `project.c`, `storage.c`, `editor.c` and `editor_dsyn.c`. These are adaptations and remeasurements of previously unintegrated persistence candidates; the old direct legacy conversion/scratch saving is already integrated and is not counted again.

The changes share historical header/length/hash validation and current-project initialization, A/B copy winner selection, bank validation, the settings snapshot/write/cache transaction, and editor step-change WATCH shadow bookkeeping. Their format-specific conversion, first-valid-copy retry, short-circuit length checking, empty-bank handling, IRQ boundaries, selected-track ownership and caller error/refusal semantics remain in place. No historical format, protocol command or factory sound is removed. The actual .67 batch recovers **352 B**, without changing any RAM region.

The [regression patch](../../build/sustain-headroom/cold-sharing-regression.patch) adds a real production-code/fault-injected NOR fixture. It passes the original baseline and candidate. It exercises the actual backup begin/data/commit, transport refusal, side-write failures and deferred retry, upstream settings suffix and restart, sample upload scratch boundaries, and actual MICRO/FILL/LOCK/DRUM requests for selected/nonselected WATCH signatures. Added SYN STORE checks cover busy/flash/staging refusal, write failure preserving `persist_saved`, and successful retry. No expected result was relaxed.

The full candidate also passes existing `project_test`, `storage_test` (torn writes), `upreset_test`, `upstream_project_test`, `section_save_test`, `delay_storage_test`, `chord_voicing_storage_test` and `dsyn_test`. See [combined persistence validation](../../build/sustain-headroom/combined/validation/result.json) and [actual failure-path log](../../build/sustain-headroom/combined/validation/headroom_commit_test-run.log). Its smaller persistence-only predecessor passes the same tests. Target budgets/inlining pass for the no-additional-RAM combination too.

### Startup string packing

[Boot-string patch](../../build/sustain-headroom/boot-strings.patch), independent of the cold-sharing and LZ4 patches. It changes `tools/build.py`, `tools/pack_boot_tables.py`, adds `tools/pack_strings.py` and `firmware/src/boot_strings.c`, and adds two NOLOAD input ranges to the existing BSS/pool output sections in `firmware/app.ld`. No parent-active voice/envelope/engine/parameter/UI source is edited.

This is a target-object transformation after the normal C compile, before final linking. It inspects the installed compiler's actual ELF32 mergeable literal-string section. Complete literal objects are suffix/substring pooled deterministically; their complete bytes, including terminators and any embedded zero bytes, are preserved. Longest roots are found first and emitted in lexical order. The selected pool is **9,414 bytes**, split at a literal boundary into **6,485 B main RAM / 2,929 B pool**. Final linked alignment accounts for the larger occupancy deltas above.

Each original string symbol is assigned its corresponding pool offset. Its existing references remain ordinary compiler/linker relocations; the final linker resolves data pointers and pi32v2 instruction relocations to the restored RAM addresses. No linked absolute address is baked into a packed stream. The two literal sections become NOBITS/NOLOAD destinations. Two independently compressed streams remain in XIP, totaling **7,507 B** for the complete candidate. The existing C boot decoder expands them once at the end of `boot_tables_init`, before zones/presets, recovery/normal startup, interrupts or core 1 launch. Ordinary host builds retain their original C literals and bootstrap behavior.

The generator checks the expected section type/flags/alignment, literal extents, accepted relocation types (5/6/8), zero string relocation addends, complete literal preservation, no literal crossing the split, and pool sizing. Unsupported object layouts fail the private build instead of guessing. This implementation is tied to the installed JieLi ELF32 contract; a future toolchain change requires rerunning these structural checks. It uses the existing decoder and does not compress audio, sample bytes, project records, code or live DSP tables at runtime.

**Qualification boundary:** the link and byte/reference proof are complete, but this new startup mode has not run on hardware. Cold and recovery boot/watchdog timing and actual fault-display availability during startup need qualification before release. No full candidate UI/audio-golden suite is claimed for the ELF string-address transformation. The meaningful evidence is the actual decoder output plus all final linked literal symbols, not ordinary host tests that still use raw C literals. Keep the 464 B path available if the RAM cost or boot qualification is unwelcome.

### Complete candidate validation and artifacts

- Actual C restoration under UBSan reproduces all **9,414 pooled bytes**, after poisoning both destinations and restoring three times.
- For **all 1,360 complete literal symbols**, the final linked ELF address points into the intended BSS or pool range and the bytes at that reconstructed address exactly match the corresponding original pre-transformation object. The verifier compares complete symbol sizes, not only the prefix before the first zero. No such literal remains loaded in XIP.
- The **303,054 B factory sample payload** remains byte-identical to baseline (same hash given above).
- Existing LZ4 contract, boot table decoder/bounds, raw/boot presets and raw/boot startup-state tests pass on the complete source. All 171 canonical factory records/names and state/zone outputs remain byte-identical.
- Target placement, ROM-reference, DSP budgets and hot-inlining checks pass. RAM flash code remains 925 instructions without calls; core-1 loop remains 60 B.
- Final full build repeats with the same image hash. Original frozen firmware/tests/assets hashes remain unchanged; only new private test files were added.

[Literal/decoder proof](../../build/sustain-headroom/combined/validation/strings-result.json), [verification driver](../../build/sustain-headroom/combined/validate-strings.py), [metadata logs](../../build/sustain-headroom/combined/validation/), [complete source patch](../../build/sustain-headroom/complete-headroom.patch). Patch application was checked against a clean private copy of the frozen source. Generated headers and object/binary files are evidence and are not part of the source patch.

To integrate only the no-additional-RAM path, apply `cold-sharing.patch`, `optimal-lz4.patch`, and the regression patch, then regenerate normally and measure the actual sustain source. For the larger candidate, `complete-headroom.patch` contains the source changes for all three components; add the regression patch separately. Do not apply component patches and the complete patch together.

Private final reproduction:

```powershell
python build/sustain-headroom/combined/generate.py
wsl -d Ubuntu --cd /mnt/c/code/sloop-fm1/build/sustain-headroom/combined --exec python3 run.py combined
wsl -d Ubuntu --cd /mnt/c/code/sloop-fm1/build/sustain-headroom/combined --exec python3 validate-strings.py
wsl -d Ubuntu --cd /mnt/c/code/sloop-fm1/build/sustain-headroom/combined --exec python3 validate-persistence.py
wsl -d Ubuntu --cd /mnt/c/code/sloop-fm1/build/sustain-headroom/combined --exec python3 validate-metadata.py
```

Run sequentially against that private root. `validate-strings.py` intentionally reads `results/combined/`; preserve that named result when testing a different label. Original and transformed objects are both retained in the private build directory; the packer rewrites only the ordinary intermediate object, not source or a shared file.

### Other measured trials

Engine descriptors were also packed with a relocation-aware token dictionary, excluding the parent's active LOFI source. The real trial saves **336 B** but adds **2,016 B main RAM** (91,472 B total). It is not included in either delivered combination: string packing uses the available RAM more effectively and adding both would leave very little main margin. The simpler dictionary carries 146 resolved pointers plus a 968-byte payload for 2,024 descriptor bytes; raw zero-pointer compression ratios overstate its value. This rejected evidence is retained under `descriptors/results/descriptors/`.

An initial unpooled all-string trial saves 920 B while consuming more RAM than the suffix-pooled result; it is superseded. Sorting/packing payload counts are screening evidence only; the recommendation uses the complete linked 2,276 B result. The final gain is not the sum of 352, 112 and the independent 1,708 B string row: compiler/layout and exact representations differ.

## Production integration

Cold sharing and its fault-injection regression are integrated after the minimum-cost LZ4 parser already shipped. Startup-string packing is intentionally excluded. The combined sustain/weighted-voice/FM6/readmission-fix build is **583,776 B**, **560 B free**, versus .68 at 584,064 B. Main/low/pool static RAM remain 89,616 / 28,928 / 331,852 B. This is a combined 288 B saving, not a remeasurement of the isolated cold-sharing patch. Tests live in `tests/headroom_commit_test.c` and the maintained runner.
