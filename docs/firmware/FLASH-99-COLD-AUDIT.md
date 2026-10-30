# Cold code headroom audit after UI reuse

The recommended packing/hash patch is now integrated. The default-DUCK fixture mismatch identified below is also corrected by omitting optional metadata at default settings. [Combined .100 build and new presets](CONSOLE-SOUNDS-EXPANSION.md) records actual final capacity and validation; independent savings below retain the frozen .99 baseline.

This audit freezes the current dirty `.99` working sources, not Git HEAD. It does
not edit production sources, shared generated/build outputs, the version, or the
installer. The snapshot and all experiments are under `build/flash-99/cold/`.
The 360 original firmware/tool/test files in its manifest remain unchanged.

The earlier `.91` project conversion/storage sharing and `.99` UI wrappers,
parameter arithmetic, menu stepping and held-page dispatch are already present;
none of their savings are counted again.

## Measured recommendation

Apply `build/flash-99/cold/cold-sharing.patch`, subject to the parent's combined
build and regression checks. It saves **80 B** in the actual pinned normal target
link, without additional static RAM or removed functionality.

| Isolated source | Linked image | Flash free | Change from baseline |
| --- | ---: | ---: | ---: |
| Frozen `.99` baseline | 583,728 B | 608 B | — |
| Word-based drum-setting packing | 583,696 B | 640 B | -32 B |
| Above plus shared byte checksum | 583,664 B | 672 B | -64 B |
| Above plus direct destination packing, recommended | **583,648 B** | **688 B** | **-80 B** |
| Extra cold-reader noinline attributes, rejected | 583,680 B | 656 B | -48 B |

All five links retain main static RAM **97,612 / 98,304 B**, low RAM
**28,928 / 29,392 B**, and pool **335,424 / 344,064 B**, including the 8,192 B
reserve. The application limit remains 584,336 B. These are sequential combined
measurements; individual savings are not presented as independent additive wins.
Normal `-Oz`, production diagnostics disabled, and pinned
`jieli-linux-toolchains-20250324.1` were used throughout.

### Drum project settings

The existing TUNE/DECAY representation is 16 seven-bit values in seven 16-bit
words. Replace the per-bit put/get loops with word shifts, handling a second word
only when a seven-bit value straddles the boundary. Clear and fill the actual
destination words instead of filling a 14 B local array and copying it afterward.
The last lane starts at bit 105, shift 9, and does not read/write an eighth word.
Masks, signed decoding, checksum, corrupt-record defaults and the on-disk layout
are retained exactly, including arbitrary stored byte values outside UI ranges.

The put function falls from 156 to 114 linked code bytes. Its target local
activation falls from 56 B (saved registers plus 16 B local allocation) to 28 B.
The getter stays inlined in `proj_apply`; that function falls from 1,824 to
1,800 B. These are local disassembly observations, not device stack high-water
measurements. The bounded bit loops become bounded word operations; no audio
processing path changes and no new calls are introduced by this part.

### Exact shared byte hashing

Project checksums and the UI's raw step-cell invalidation hash use exactly the
same byte-wise FNV-1a seed and recurrence. Move the existing `proj_hash` helper to
the shared integer-helper translation-unit section in `dsp.c`, retaining its
historical name for the existing project test/API references. `steps_hash` tail
branches to it. No hash seed, cell length, string hash, word hash, editor lock
hash or redraw ownership policy changes.

The shared hash body remains 30 B; `steps_hash` falls from 38 to 12 B, and the
full link saves another 32 B including layout. Its tail branch adds no nested
frame; the shared hash has its existing 8 B register save. This introduces one
bounded cold UI tail branch, not a sample-loop call. Hardware UI speed and total
stack high-water are unmeasured.

## Verification

Baseline and recommended candidate both pass:

- **200,000** arbitrary-byte project packing/unpacking/hash oracle cases against
  the original frozen per-bit functions, followed by the original project format
  suite. Entire project objects are compared, so accidental adjacent writes are
  visible; corrupt checksum cases are included.
- The **42,000** legacy import/output/rejection corpus, unchanged digest
  `8b386c47`.
- Production persistence commit/fault/retry/cold-load checks.
- Snapshot cold restart, section slots, queued capture, A/B-only autosave,
  trailer faults and legacy/mismatch rejection.
- UI refresh dirty/full parity, palettes, drum-cell union, modulation state,
  record/song layout and help checks. All **38** captured framebuffers match
  byte-for-byte.

These focused host programs run with nonrecovering UBSan, except the existing
fixed-point signed-shift exception. Full audio golden and combined parent
integration qualification remain for the parent; no hardware timing claim is
made. Target placement and RAM/region checks pass in each measured link.

The first private oracle compile omitted `<assert.h>`; it was corrected and
rerun. The first baseline UI refresh run lacked its output directory; after
creating it, the run passed. Initial and rerun logs are retained separately.

### Pre-existing upstream fixture assertion

`upstream_project_test` fails its raw load/apply/recapture `memcmp` on the
unmodified frozen baseline as well as the candidate. The import itself is
byte-identical. In both `upstream271.fun6` and `upstream28.fun6`, recapture changes
only payload byte **65**, the high byte of `g[G_DUCK]` (id 28, offset 64), from
`00` to `42`, plus checksum bytes **3812–3815**. The `.98` DUCK writer always
adds tag `0x4000` and default recovery `1 << 9`, producing `0x4200` at depth zero.

A separate private diagnostic verifies all original semantic arp/continuation/
legato/drum-tune assertions, depth zero, and trigger/recovery/shape defaults
0/1/0. Normalizing only that optional metadata word and recomputing its checksum
restores full-record byte equality. This is an existing test assumption about
reserialization after new optional metadata, not a failure introduced by these
refactors. The original production/test assertion is untouched; the parent
should decide how to update its compatibility oracle separately. See
`upstream-diag.log` and `upstream-normalize.log`.

## Rejected and unmeasured work

Adding size/noinline attributes to the remaining two project readers grew the
recommended image by **32 B**, despite making `proj_apply` smaller in isolation.
That illustrates why function-size estimates are insufficient; it is excluded.

Already shared storage gather/extension writing, legacy conversion bodies,
parameter enum stepping, and packet pack7 helpers offer no newly demonstrated
saving here. Differing checksum seeds, word-vs-byte hashing, bounds checks,
header commit ordering and UI state policies were not treated as interchangeable
duplicates. Broader protocol/editor dispatch changes and panel permutation
helper extraction remain unmeasured, not promised savings.

## Evidence

`build/flash-99/cold/` contains `baseline-manifest.json`,
`frozen-source-check.json`, five private source/build trees and target logs,
`cold-sharing.patch`, `symbols.json`, `checks.json`, `rerun.json`,
`framebuffers.json`, the frozen-reference `cold99_oracle.c` in the test trees,
and all original/rerun compatibility diagnostic logs. No production source,
shared build output, version or installer was changed by this audit.
