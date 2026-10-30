# .91 storage, persistence and protocol flash audit

Frozen-source audit, 2026-10-28. Production sources, formats, assets, Git state and partition map were not changed. All experiments and artifacts are under `build/flash-audit-91/storage/`; the read-only baseline is `build/flash-audit-91/baseline/source/`. This report is the only delivered file outside the private directory.

The strongest tested combination recovers **272 bytes net**, linking at **584,024 / 584,336 B** (312 B free). All static RAM regions are unchanged. This is useful incremental headroom, not a multi-KiB solution. Four independent target links and two actual combined links support the result; the reported combination is not a sum substituted for linking.

## Baseline and prior-work reconciliation

Read ARCHITECTURAL-HEADROOM.md, ENGINE-PERFORMANCE-INTEGRATION.md, HEADROOM-COMPARISON.md, LITERAL-STORAGE.md and OPTIMIZATION.md first. Their older measurements are historical, not this baseline. Already implemented: sparse/bit-packed startup presets and names, shared A/B selection, commit-last extension verification, direct FUN1/2 conversion without the former 2,584-byte temporary, scratch-backed backup/upload staging, settings/editor cold helpers and literal grammar packing. None is claimed again here. The previously regressing cold-state restoration and runtime sample-packing approaches remain excluded. No additional RAM-funded expansion is proposed.

`base/` reproduces the supplied uncommitted .91 hold-delay/worker-hardening snapshot, rather than Git HEAD. Its binary is byte-identical to the supplied baseline: SHA-256 `c98a63741a9f16f2b9f7faa753dd31c980241a7013e56121339fbb03274552ad`. Every target build used the frozen build driver and generated headers, pinned SDK checks, the specified Jieli 20250324.1 toolchain, and explicit `FELUCCA_AUDIO_DIAG=0`. Target builds were sequential. No timing claims are made from this shared host.

| Build | Linked flash | Saving vs baseline | Flash free | Main | Low | Pool |
|---|---:|---:|---:|---:|---:|---:|
| Reproduced baseline | 584,296 | 0 | 40 | 97,500 | 28,928 | 335,424 |
| Modern FUN4/FUN5 track prefix | 584,152 | 144 | 184 | 97,500 | 28,928 | 335,424 |
| Old FUN2/FUN3 conversion body | 584,200 | 96 | 136 | 97,500 | 28,928 | 335,424 |
| Storage forward byte-copy helper | 584,280 | 16 | 56 | 97,500 | 28,928 | 335,424 |
| Editor step-prefix hash reuse | 584,280 | 16 | 56 | 97,500 | 28,928 | 335,424 |
| Bitwise CRC, deferred | 584,232 | 64 | 104 | 97,500 | 28,928 | 335,424 |
| Modern + copy + hash, actual link | 584,120 | 176 | 216 | 97,500 | 28,928 | 335,424 |
| All four sharing candidates, actual link | **584,024** | **272** | **312** | **97,500** | **28,928** | **335,424** |

Capacities: main 98,304 B; low 29,392 B in this .91 linker; pool 344,064 B. Remaining: main 804 B, low 464 B, pool 8,640 B. The retained pool reserve remains 8,192 B, leaving only 448 B beyond reserve. No candidate spends it. `measurements.json` records binary sizes/hashes; each variant has its own ELF, disassembly, package and build log. Final binary SHA-256: `50d6958cecc904f13ea22c92b1a5310a5e2d9b703c83a340bfe0e416569aea16`.

## Ranked candidates and review costs

| Rank | Candidate | Independent saving | Disposition and cost |
|---|---|---:|---|
| 1 | Share FUN4/FUN5 track prefix | 144 B | Recommended. One cold helper handles parameter defaults/count mapping, engine/preset and ten-byte steps. FUN5 micro/fill/lock remapping remains explicit. |
| 2 | Share FUN2/FUN3 conversion | 96 B | Recommended. Common validated prefix and variable parameter/track stride; FUN1's special default-track behavior remains separate. |
| 3 | Reuse project FNV for editor step bytes | 16 B | Optional. Adds one small call per editor signature; retains micro/fill/lock order and values exactly. No audio callback change. |
| 4 | Share forward byte-copy loops | 16 B | Optional. Preserves forward-overlap and null-plus-zero-length semantics, unlike blindly substituting memcpy. Target save frame grows 8 B; evaluate this small saving against stack margin. |
| Deferred | Remove CRC nibble table, bitwise polynomial | 64 B | Correctness tests pass, but eight recurrence iterations replace two nibble updates per byte. Snapshot polling and large backup/sample CRCs also use this function. Not in either recommended combined image. |

Modern helper performs the same 53 common-parameter assignments, eight engine assignments and step copy. FUN5 now has a parameter-bound/default branch that its original fixed loop did not need, plus one helper call per converted track. FUN4 already had the default branch. No conversion work moves into live rendering. Old helper retains full-record length/magic/hash validation before initializing the destination, and selects only the known FUN2/FUN3 sizes/counts. Its wrapper target calls tail-jump into the shared body. No failure path now partially initializes a previously untouched rejected destination.

The editor hash change calls `proj_hash` for the ten step bytes, then continues the original recurrence for signed micro converted to uint8, fill+1, and ordered matching locks. It does not fold or serialize a different representation. The callback is a main-loop WATCH operation; no note ownership or real-time sample loop is edited.

Storage retains the same bounded forward byte operations in the same order. This matters if a future caller aliases gather source/destination: a generic memcpy replacement would change the language contract. Validation, A/B ordering, sequence choice, side/trailer programming/verification, page writes, header-last commit and readback are untouched. No ownership or lifetime of `main_scratch.st`, `proj_tmp`, `autosave_buf`, section snapshots or XIP preset-bank pointers changes.

## Target stack and runtime evidence

`frames.txt` records baseline/final prologues. These are static local frames, not hardware high-water or a full interrupt-inclusive call graph:

- `st_copy`: leaf, no stack allocation or register push. `st_load` saves one more register (4 B). `st_save3` retains the same register-save set but changes explicit locals from 72 to 80 B: **+8 B** on its existing call path. The page-verification buffer is unchanged.
- `proj_trk_modern`: saves return plus r4..r7 (20 B). FUN4 caller saves 20 B instead of 52 B; caller+helper is 40 B before deeper callees versus the old caller's 52 B. FUN5 caller loses its former 16 B local allocation, then gains the 20 B helper frame: **+4 B** at the helper boundary versus the old caller before deeper callees. These local comparisons do not establish the deepest library-copy path.
- FUN2/FUN3 shared body saves return plus r4..r10 (32 B), with no explicit local array; wrappers tail-jump. Baseline FUN2 saves 24 B, so the new body has **+8 B** there. Baseline FUN3 has separate wrapper/body frames; do not treat its wrapper alone as the whole old chain.
- Editor signature retains its 28 B register frame, and the called `proj_hash` saves 8 B: **+8 B** during that short call.

No heap, persistent buffer, startup decoder, pool allocation, new ISR work or sample conversion is introduced. Call/branch costs above are structural observations, not measured device cycles. Device stack high-water and watchdog/startup/storage responsiveness remain hardware gates, particularly with only 804 B unallocated main static RAM; static free RAM is not itself a stack measurement.

## Behavior validation and evidence

`validation-final.log` and `tests/combined-final/` contain the final combination's host evidence, compiled with the supplied optimization compiler wrapper, assertions enabled, `-O2 -fwrapv -fno-strict-aliasing`, and private include/output paths. All nineteen selected programs pass after supplying the scale UI test's required private `build/host` screenshot directory. The first scale run reached its screenshot write and failed on the missing directory; the rerun passes (`scale_midi_grid-rerun.log`), and the baseline also passes once that directory exists. This was a harness output-path issue, not a routing assertion failure.

- Legacy conversion: **42,000 complete import/output/rejection cases**, expected digest `8b386c47` unchanged. Includes all six historical/current formats, corrupted magic/size/checksum, short/long/negative lengths, and initialized-output hashing; not merely a valid-file smoke test. Independent modern/old variants and the final combination also pass ordinary project and independent upstream-produced project roundtrips.
- Storage: A/B writes, torn saves, newest-body corruption fallback, both-invalid rejection, empty records, XIP bank copy changes, side-record retention and old-save erased side behavior.
- Actual persistence/editor harness: dropped/corrupt/program/read side faults preserve old commits; deferred retries and cold reload; eight sample uploads covering four slots and 1/16 zones; invalid-header refusal, poisoned scratch, exact header/data bytes and backup restore; stopped-only restore with active transport refusal; SYN write failure/cache/retry behavior; WATCH selected/nonselected MICRO/FILL/LOCK/DRUM notifications.
- Snapshot persistence: cold restart, all four sections, queued capture, A/B-only autosave, unchanged upstream payload prefix, trailer fault/retry, legacy and checksum mismatch rejection. The existing semantic distinction between per-section snapshots and the working autosave remains intact.
- User preset parser/version/bank roundtrip, delay character project/section storage and old defaults, chord voicing persistence.
- Independent ties and ownership: sequencer note ties, sequencing ownership, phone steps, note-chain actions, USB scene transactions, MIDI UART, expression/bank and scale-grid programs. Scale grid covers all roots/scales, held pitch identity, collisions, USB/TRS, panic and admission-capacity ownership. No ownership helper was changed just to recover bytes.
- New frozen-reference editor oracle: **640,000 signature comparisons** (10,000 random complete tracks x 64 steps) against the original `ed_step_sig` body, including arbitrary micro/fill/lock bytes, then the actual persistence/editor suite. Source: `combined-final/tests/storage_hash_oracle_test.c`; output: `tests/combined-final/storage_hash_oracle.log`.
- Three focused UBSan runs cover legacy conversion, the editor oracle/actual persistence harness and snapshot persistence; see `validation-ubsan.log` and `tests/combined-final-ubsan/`.
- Target region checks pass: `.ram_text` 925 instructions/no calls, `.c1_text` 160 B, persistent scratch 6,216 B, unchanged main/low/pool accounting and HAL boundary checks. Separate DSP budget and hot-loop call-cap checks pass (`target-budget.log`, `target-inlining.log`). These static checks are not device deadline measurements.

Earlier intermediate combinations and independent candidates have their own logs (`validation.log`, `validation-all.log`). The deferred CRC trial passes storage, actual persistence and snapshot fault tests; this is not evidence that its additional main-loop work is acceptable on hardware. No full audio golden batch was rerun for these exclusively cold-code changes; existing target hot-loop checks were run. No factory asset, live DSP algorithm, overload guard or voice budget changed.

## Exact application and reproduction

Reviewable patches are private, under `build/flash-audit-91/storage/patches/`:

1. `modern-share.patch` — FUN4/FUN5 prefix.
2. `old-share.patch` — FUN2/FUN3 body; independent baseline patch, also applies after (1).
3. `copy-share.patch` — storage gather/load loops.
4. `hash-share.patch` — editor FNV prefix.

Alternatively use `combined-final.patch` alone against the frozen baseline. **Do not also apply the four standalone patches.** `crc-bit.patch` is an independent, deferred experiment; do not apply it to the recommended combination.

Patch text uses normalized line endings. Normalize the three touched files (`project.c`, `storage.c`, `editor.c`) to LF in a private copy before GNU `patch --batch -p1`. `check-patches.py` demonstrates the exact normalization and ordered application and verifies all three reconstructed files against the measured final sources (`patch-application.log`). It operates only on `apply-check/`. No Git checkout, index or commit operation is needed.

From the candidate's private source directory in WSL Ubuntu:

```sh
FELUCCA_AUDIO_DIAG=0 \
JIELI_TOOLCHAIN=/home/shaun/.cache/sloop-host-tools/jieli-linux-toolchains-20250324.1 \
python3 build/build-target.py
```

Use the frozen build driver/headers/SDK payloads; do not regenerate assets or substitute current production source. `validate-final.py` drives the selected host programs; `validate-ubsan.py` adds sanitizer checks. Create that source copy's `build/host` before scale-grid runs. `measurements.json` and the per-variant target logs are the accounting oracle.

## Larger opportunities examined, without unsupported savings

No multi-KiB claim is justified by this specialty's remaining obvious duplication. These are **unmeasured design directions**, not linked gains:

- A table-driven multi-format project decoder could share more complete initialization/finalization and parameter mapping across FUN1..5, perhaps with the user-preset count mapper. FUN1 defaults/selection, pre-FUN4 swing scaling, drum note-to-lane conversion, FUN4 direct ten-byte steps, FUN5 locks/micro/fill and current format suffixes are semantically different. The tested pairwise sharing above avoids turning all formats into an interpreted schema. A larger implementation must beat its descriptor/access code and rerun the complete legacy byte oracle before claiming savings. Host `UP_HOST` also intentionally builds a smaller independent surface, so casually routing presets through project conversion breaks that boundary.
- A common backup-object validator/commit descriptor could reduce editor dispatch duplication, but objects have different busy checks, zero-length rules, RAM/XIP staging, validity checks, refresh callbacks and deferred settings durability. No branch was proved unreachable merely because a normal UI does not use it. Existing older editor/upstream/recovery paths must stay.
- Project capture/apply and snapshot capture/restore are not duplicate serializers that can be merged by reversing memcpy direction: they have different parameter widths, normalization, lock bases, per-track identities and section/working lifetimes. A direction flag in inner loops may cost more and must not move persistent ownership into shared scratch.
- Additional cold-code compression/overlays need independent RAM, call-graph and loader work; the current 804/464/448 B usable main/low/pool margins do not fund an assumed decoded image. No overlay, partition, retained-reserve spending or runtime sample packing is proposed.
- MIDI note-chain edit/place/pitch operations already share a bounded action helper. Independent per-pitch tie ownership and scale-held source identity are correctness state, not redundant data. Removing those checks/state or aliasing them with USB upload scratch would fail this audit's preservation constraints.

Recommended integration scope is the two conversion helpers first, with the 16 B hash/copy changes considered separately against their explicit call/stack cost. The **272 B** figure applies only to the complete measured four-candidate combination. Any production integration, including other specialists' patches, needs a fresh combined link and the same format/fault/ownership gates. Hardware cold boot, import/backup with USB traffic, repeated stopped save/autosave, actual power interruption and stack/watchdog responsiveness remain unverified on a device.
