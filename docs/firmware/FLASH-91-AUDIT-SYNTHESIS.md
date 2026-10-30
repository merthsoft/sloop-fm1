# Flash audit synthesis against .91

All six specialist audits completed against the frozen working-source .91 build,
including the uncommitted worker hardening and 350 ms hold threshold. Each
reproduced the 584,296-byte baseline exactly. Production was unchanged during
those audits. The first three candidates are now
integrated and measured together: [integration evidence](FLASH-91-INTEGRATION.md).
Firmware .92 leaves 2,012 bytes free in the unchanged 584,336-byte application
slot. The tables below preserve the original independent audit results.

## Measured candidates

These are separate specialist combinations, each measured against .91. They have
**not** been linked together; their savings must not be advertised as an achieved
cross-audit total.

| Area | Measured saving | RAM change | Assessment |
| --- | ---: | --- | --- |
| [UI](FLASH-91-UI-AUDIT.md) | 1,488 B | None | Largest candidate: fixed-font/palette calls and full-width canvas operations share small helpers. 117 framebuffer captures match; navigation, clipping, hold timing, fuzz and UBSan pass. Adds UI wrapper calls; hardware stack/frame responsiveness remains to check. |
| [Storage](FLASH-91-STORAGE-AUDIT.md) | 272 B | None | Share modern/legacy import bodies, forward copies and editor hashing. Conversion/output/rejection and storage fault oracles pass. The copy/hash slices each save only 16 B and have small call/stack costs; they are optional. |
| [Startup data](FLASH-91-DATA-AUDIT.md) | 216 B | None | Order synthesized-drum dictionary rows for compression and extend literal grammar with unused control-byte tokens. Exact original sound selections and restored bytes/addresses pass. Changes startup work, not playback decoding. Full integrated suites and hardware boot qualification remain. |
| [DSP](FLASH-91-DSP-AUDIT.md) | 208 B preferred | None | Share PHYS finalization/dispatch, accumulator clearing and limiter output. Exact state/audio, 200 goldens, sanitizer and split checks pass. Adds block-boundary calls; no per-sample helper. Qualify deadlines before adoption. |
| [Target code generation](FLASH-91-CODEGEN-AUDIT.md) | 320 B | None | Application-only inline threshold zero, with timer/IRQ primitives explicitly kept inline. Monitored DSP/ISR metrics stay equal, but some other event/mixer setup acquires calls. Keep separate for hardware comparison. |
| [RAM-funded literals](FLASH-91-RAM-AUDIT.md) | 236 B | +512 B main, +400 B pool | Measured combination leaves 292 B main free and only 48 B pool beyond the reserve. Defer: poor exchange for remaining allocation margin. Alternative data-audit RAM trade also overlaps this mechanism. |

The optional DSP FORMANT helper raises that specialist's measured saving to
240 bytes but adds approximately 24 bytes to the inspected setup call chain.
Its 288-byte alternative grows the sample-loop span. Neither is the preferred
initial batch.

## Recommended integration order

1. Integrate the UI combination alone. Review wrapper calls and runtime palette
   reads; repeat target link and exact framebuffer/control tests.
2. Add storage sharing. Preserve import failure behavior, autosave/snapshot source
   lifetimes, forward-copy semantics and all commit-last guarantees. Rebuild and
   run conversion, persistence and interruption tests.
3. Add the two no-RAM data changes. Regenerate drum tables from source, remapping
   every dictionary index, and land grammar encoder/decoder/tests together. Run
   original-sound and complete literal/table restoration oracles, then full
   audio/UI suites and installer checks on the resulting combination.
4. Evaluate the preferred DSP batch separately with the actual GRAIN/VOICE/drone
   and master-LOFI workload. Preserve the noinline modal-kernel boundary and
   compare deadline/stack behavior; do not infer a speed improvement from size.
5. Test the guarded compiler-option candidate separately after the source changes
   settle. Its independent 320-byte gain may change when combined with outlining.

Keep the current RAM margins and all existing features/sample quality. The first
three candidates add no persistent RAM or audio-loop work, but UI wrapper calls
and startup/conversion work still have runtime/stack costs. No host result
qualifies physical cache ordering, watchdog margin or worst-case audio deadlines.

## What the audits ruled out

- Linker garbage collection found no large dead-code windfall; its baseline
  image was byte-identical. Data sections plus GC saved only four bytes.
- No worthwhile new assembly candidate was established. Blanket native-clamp
  replacement grew the image by 32 bytes; jump-table/global-merge experiments
  overflowed the real slot.
- Current LTO is blocked by the metadata/literal pipeline expecting ELF32
  objects. Earlier vendor LTO capability does not establish a current size win.
- The former 2,584-byte conversion scratch is already removed; it is not a new
  RAM opportunity. Narrowing/reordering track fields recovered only 80 RAM
  bytes while adding 2,256 flash bytes and was rejected.
- Direct autosave-buffer/scratch aliasing was demonstrated to commit the wrong
  payload. That is a counterexample to a proposed optimization, not evidence of
  corruption in the unchanged production layout.
- Startup-decoded labels/tables remain live. Their storage cannot later become
  scratch. No runtime sample compression, feature removal, reduced audio quality,
  weakened protection or repartitioning is proposed.

Larger architectural UI recipes and arena-backed derived caches remain bounded
research ideas, without measured multi-KiB savings or proven ownership yet.
Specialist reports retain private patches, application order, actual binary
measurements, tests and rejected alternatives under `build/flash-audit-91/`.
