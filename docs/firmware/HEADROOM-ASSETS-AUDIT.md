# Immutable asset and catalog headroom audit

## Approved generator integration follow-up (2026-10-23)

The parent subsequently authorized production edits for **only** the fixed font
metrics and ragged preset trims. These are now implemented through their actual
generators. The analysis-only statements below describe the earlier experiments;
zone and additional numeric boot packing remain experimental and unintegrated.

Changed files: `tools/gen_font.py`, `firmware/src/gfx.c`, `tools/level_presets.py`,
`firmware/src/preset_trim.h`, `tests/font_asset_test.py`, `tests/font_render_test.c`,
`tests/font_test.c`, and new `tests/font_generation_test.py`. No shared `build/gen`
output was regenerated. No engine implementation, test runner, version, package,
Git ref, or device was changed by this specialist.

The real font generator independently selects scalar or array storage for each
metric. Nonuniform advances/bitmap widths retain their existing arrays. Rendering
and width calculation use `font_advance()` / `font_bitmap_width()`. Fixed scalar
fields occupy the existing descriptor's target padding. The asset oracle now
accepts both descriptor forms, changes the bitmap format field explicitly when
building legacy fixtures, and accepts an optional private output directory.
Standalone C font tests include their required test-local string helpers.

`level_presets.format_header()` emits a flat signed trim array with uint16 offsets,
uint8 counts and a bounded inline `preset_trim_value(e, pi)` accessor. It preserves
every engine/preset/name/value comment so the existing name-based loudness carry
forward continues to work. Interior holes retain their index; named zero values
are retained; only unnamed zero tails are omitted. All 157 existing named trims,
including the optional engine, and their 416-byte expanded gain table are unchanged.
No loudness measurement was rerun. Scientific dependencies load only when actual
loudness analysis is requested, allowing pure generation tests without SciPy.

**Required parent/engine-owner hook:** in `firmware/src/engines.c`, replace only
the body expression of the existing `preset_trim(uint32_t e, uint32_t pi)`:

```c
return preset_trim_value(e, pi);
```

The old `PRESET_TRIM[e][pi]` caller is incompatible with the new flat header until
that hook lands. This specialist deliberately did not edit the engine-owned file.
There is one additional parent-owned test hook: in `tests/stress_test.c:38`,
replace `FONT_S.bw[gi]` and `FONT_L.bw[gi]` with
`font_bitmap_width(&FONT_S, gi)` and `font_bitmap_width(&FONT_L, gi)` respectively.
Its independent enlarged bitmap oracle otherwise dereferences null metric pointers
when consuming the new generated fixed-width font. This specialist did not edit
the unowned stress test.
Parent-owned `tests/run_tests.sh` should additionally run
`python3 tests/font_generation_test.py`; existing font test invocations continue
to work. Parent must regenerate normal shared assets and measure the final
combined target after all approved integrations. The earlier isolated −144/−192
byte figures are **not** claimed as newly measured final generator-backed deltas.

Validation completed against private outputs in
`build/headroom-assets-51/integration/`:

- `python tests/font_generation_test.py`: **5 tests pass**, including real fixed
  metric generation, synthetic proportional generation, invalid metric bounds,
  trim holes/empty rows, deterministic header regeneration and the frozen
  expanded gain-table SHA-256.
- `python tests/font_asset_test.py build/headroom-assets-51/integration/font-assets`:
  **all 224 BDF glyphs match**, with unchanged 3,952 bitmap bytes.
- Actual C `font_render_test`: **6,120 pixel/metric comparisons pass** for both
  one-bit and independent four-bit fixtures. A synthetic varying-advance AND
  varying-width font additionally checks exact pixels and total advance.
- Actual C `font_test`: all historical glyph/width hashes, clipped captures,
  Latin-1/capitalization and anti-aliased fallback checks pass against the actual
  newly generated header.
- Native C exhaustive `preset_trim_value` test: **525 engine/preset combinations**
  plus UINT32_MAX boundaries equal the original frozen expanded gain table.
- `git diff --check` passes for the owned implementation files.

Native tests use the installed Zig 0.13 compiler under WSL, copying only owned
sources/generated assets into private paths; logs and executables are retained
under `integration/`. No final combined target, audio golden suite, UI fuzz or
hardware startup result is claimed here; those remain parent integration gates.

2026-10-23. Analysis only, against a frozen copy of the **working source after
the two-icon cleanup**, not HEAD `9fd9ddc`. No production firmware, shared
generated assets, shared build outputs, Git refs, package, or device changed.
Evidence and experimental patches: `build/headroom-assets-51/` (ignored).

## Ranked findings

There are several small, real representation wins, but this audit does not find
another multi-KiB unused asset comparable to the completed icon cleanup.
The following are **separate, same-version target comparisons**. Do not add their
savings: no combined target was measured.

| Integration priority | Separate experiment | Image bytes | Net flash saved | Main static RAM | Main RAM delta |
| --- | --- | ---: | ---: | ---: | ---: |
| — | Frozen baseline | 581,296 | — | 89,128 | — |
| 1 | Fixed font metrics, retain proportional fallback | 581,152 | **144** | 88,680 | **−448** |
| 2 | Ragged factory preset trims | 581,104 | **192** | 89,128 | **0** |
| 3 | Existing boot packer on grain window and FM6 envelope table | 580,848 | **448** | 89,944 | **+816** |
| 4 | Boot-expanded factory sample-zone metadata | 580,768 | **528** | 90,552 | **+1,424** |
| Reject as a batch | Pack page, groove and factory-drum-kit descriptors | 581,280 | **16** | 89,128 | **0** |

All retain **28,280/29,496 low RAM** and **332,876/344,064 pool**, including the
8,192-byte pool reserve. Main RAM capacity is 98,304. These are static linker
measurements, not free heap or measured stack margins. Raw newly materialized
arrays can differ from linked RAM deltas because of alignment.

Section deltas match the table: `.text` changes by −144 / −192 / −448 / −528 /
−16 bytes respectively, and `.bss` by −448 / 0 / +816 / +1,424 / 0. `.data`,
`.ram_text`, `.c1_text`, `.lowram`, `.pool` and `.noinit` sizes are unchanged in
all five comparisons. ELF symbol/string-table changes are debug/link metadata,
not installed application bytes; full section listings are retained per result.

Each target passes `build.check()`, `tests/target_budget.py`, and
`tests/target_inlining_test.py`. RAM flash-driver code remains 925 instructions
with no calls; core-1 waiting code remains 60 bytes. No per-sample decompressor,
cache, audio helper call, or changed sample stream is introduced.

## Freeze, tools and evidence

`source/` privately copies firmware, tools, tests, assets, and `build/gen`.
`manifest.json` hashes those inputs. Existing generated headers were copied to
preserve the current cleanup exactly. Each experiment restores the frozen
inputs before applying its changes. Baseline reproduction is exactly
581,296 bytes, with the supplied 89,128 / 28,280 / 332,876 RAM figures.

Target builds use WSL Ubuntu and
`/home/shaun/.cache/sloop-host-tools/jieli-linux-toolchains-20250324.1`, the normal
`build_app()` flags, default version/identity, and `FELUCCA_SLICE=0`. Only the app
compile/link/check path is invoked; no package or loader publication occurs.
Native equivalence checks use Zig 0.13.0, as the existing optimization wrapper
does. SDK packaging is not part of these representation comparisons.

`results/{baseline,font-fixed,trim,extra-boot,zones,padding}/` contains complete
target ELF, binary, disassembly, symbol/section listings, checks, DSP budgets,
inlining checks, and `candidate.patch`. `inventory.json` scans **baseline linked
objects**; `detail.json` scans **baseline unlinked object sections/relocations**.
The latter's compression figures are screening estimates, not linked savings.

Experimental patches deliberately prove frozen representations. They are not
ready-to-merge generator changes: see the integration tasks for each candidate.
`experiments.py` is the complete reproducible transformation, including generated
inputs; the zone experiment also copies the private `pack_zones.py` tool.
Generated headers are private to this audit, including the boot-presets probe.

## 1. Fixed font metrics

Source: `tools/gen_font.py`, `firmware/src/gfx.c`, generated
`felucca_font.h`, `boot_tables_init.h`.

Current small-font advance and bitmap width are uniformly 8 and 12 across all
224 characters. Large-font values are uniformly 16 and 24 across 64 characters.
The small arrays already compress to only 11 flash bytes each, but expand into
**448 persistent RAM bytes**. Their two boot descriptors cost another 32 bytes.
The two large metric arrays still occupy **128 flash bytes**.

The prototype adds `fixed_adv` and `fixed_bw` in the font descriptor's existing
target padding. A null metric pointer selects the scalar; nonnull pointers retain
the proportional-font representation. Both font descriptors remain **28 bytes**
on target. Removing the four constant arrays and two boot-table entries produces
the measured **144-byte net flash / 448-byte RAM reduction**, including renderer
branch/code costs. No glyph data, offset, character range, advance, clipping,
alpha level or scaled bitmap changes.

Validation: baseline and candidate pass the font oracle for every character,
four colors and six clipped positions, uppercase/Latin-1 BDF pixels, lowercase
capitalization, all widths, and the anti-aliased four-bit fallback rendering.
The standalone existing font test currently omits the string helpers required
by a newer gfx function; `validate.py` supplies test-local `str_len`/`str_cpy`
helpers in both runs, then restores the test. No production helper was changed.

Integration: teach `gen_font.py` to emit scalars when **all** entries match,
otherwise emit the existing arrays with zero scalar defaults. Do this before
`pack_boot_tables.py`, so removed arrays cannot be retained by boot descriptors.
Update independent font-test accesses to accept both representations. Run actual
UI captures/fuzz after integrating the generator change. The prototype validates
the renderer/metrics representation; no full candidate UI-fuzz run is claimed.

## 2. Ragged preset trim rows

Source: `firmware/src/preset_trim.h`, `preset_trim()` in `engines.c`,
`tools/level_presets.py`.

The existing `int8_t[13][32]` uses **416 bytes**, including implicit zero tails.
The initialized row counts are
`22,22,8,8,11,7,14,8,8,8,26,11,4`. They include the optional engine's existing
trims; no rows are discarded. The prototype stores only initialized values,
plus a 13-entry uint16 offset array and uint8 count array. Out-of-range engine
and preset indices still return zero. This is a cold preset-selection lookup,
not an audio-sample decoder or changed preset payload.

The complete target saves **192 bytes**, with unchanged RAM. The standalone C
oracle compares every engine 0..14 and preset 0..34 (**525 combinations**), plus
UINT32_MAX engine/preset boundary cases, against the original expanded table.
All trims, including implicit zero tails, match.

Integration: update `level_presets.py` to emit offsets/counts/flat values, retaining
its `/* engine.preset name */ value` comments. Its existing old-value parser uses
those comments to carry prior loudness measurements forward; the deliberately
minimal experimental header drops them and **must not be integrated verbatim**.
Do not rerun loudness measurement or alter gains to obtain this saving.

## 3. Additional startup numeric tables

Source: `GR_HANN` in `eng_grain.c`, `FM6_STATICS` in `fm6_core.c`, and the existing
`tools/pack_boot_tables.py` / `firmware/src/boot_tables.c`.

These source-defined tables were outside the generator's current filename list.
The existing predictor/bit packer works without a new format or decoder:

| Table | Expanded bytes | Packed bytes | Existing encoding |
| --- | ---: | ---: | --- |
| `GR_HANN[257]` | 514 | 106 | third differences, 3-bit residuals |
| `FM6_STATICS[77]` | 308 | 189 | first differences, 19-bit residuals |
| Total | **822** | **295** | **527 gross data bytes saved** |

The real target saves **448 bytes net**, consuming **816 additional linked main
RAM bytes**. Two new descriptors cost 32 bytes; code/alignment account for the
remaining difference from the raw payload calculation. The tables are restored
before interrupts/audio/core 1 start and thereafter remain ordinary direct-read
arrays. Runtime DSP reads the same types and bytes. No scratch array is added:
the existing decoder writes each final destination directly, with scalar local
state. Persistent destinations live for the whole boot.

The actual C initializer reproduces all **822 bytes**, even after poisoning the
destinations and initializing three times. The oracle also matches the original
linked baseline arrays. Target hot-loop budgets and call caps pass.

Integration: move these numerical definitions into a deterministic generated
header (or teach the current generator to emit them), then extend the **existing**
boot-table generation path. The proof uses `audit_extra.h` and a second tiny init
list only to isolate the comparison; a production implementation should use one
ordinary boot list. Regenerate tests' oracles and remeasure the final single-list
implementation. Do not infer identical net bytes from this prototype.

Also screened `FM6_LFO_DELTA[100]`: 400 bytes raw, LZ4 403. It fails the existing
packer’s >=112-byte gross-benefit gate even with predictors, and remains raw.
Repacking `FM6_STATICS` does not change its envelope values or timings.

## 4. Startup-only sample-zone metadata

Source: `smp_zone_t` / `SMP_ZONES` in `eng_sample.c` and `gen_samples.py`;
private object reader in `pack_zones.py`.

The 51 immutable factory zone descriptors occupy **1,428 bytes** and contain no
pointers or relocations. They compress to **854 bytes** using the existing LZ4
boot decoder. The prototype extracts the ordinary target probe's actual bytes,
asserts the 28-byte ABI and zero relocations, emits an 854-byte payload, and
restores `SMP_ZONES` into ordinary main RAM once before audio initialization.
Final target saving: **528 bytes**, with **+1,424 linked main RAM bytes**.
This leaves **7,752 main static bytes** of the 98,304-byte region.

This is **metadata expansion**, not audio expansion. All **303,054 encoded factory
sample bytes** remain in XIP and are SHA-256/byte-identical in every measured
target image. SAMPLE/GRAIN/drum reads and sample selection continue to use
ordinary `smp_zone_t` values and direct sample bytes. All 1,428 restored metadata
bytes match baseline, including predictor, loop state, rates and key ranges;
repeated C initialization passes after destination poisoning. No full audio-golden
suite or hardware startup-time measurement is claimed for these prototypes.

There is no new whole-table scratch: expansion writes the persistent destination
directly. The expanded table consumes **1,428 raw RAM bytes for the entire boot**,
unlike the already-shared `proj_tmp` scratch used for preset restoration. Low RAM
cannot hold this table at the current 1,216-byte remaining capacity. Pool reserve
is unchanged and must not fund it.

Integration: add a generated raw-vs-boot conditional to `gen_samples.py`, guarded
startup call, and explicit final-build flag. The private proof modifies the frozen
generated header, uses a fixed 51-row assertion and calls inside the existing
boot-preset block. Production should derive the row count, independently guard
the init call, include the private reader as a supported build tool, test raw
host/probe and boot paths, and regenerate from scratch. Boot latency/watchdog
still needs hardware checking. The source-layout probe is same-version; no
linked-address extraction is used to build the candidate.

## Descriptor, pointer and padding accounting

Baseline linked object inventory includes **335,673 named immutable bytes** across
363 nonzero object symbols. It is not a full image accounting: strings, functions,
alignment and unnamed constants have separate attribution. Largest retained data:
sample bytes 303,054; packed drum bases 2,831; packed preset fields 2,551; packed
font glyphs 1,968; zone metadata 1,428; drum kits 1,408; preset names 1,384;
track descriptors 1,136; global descriptors 944; pages 564.

The baseline unlinked object contains **1,751 relocations owned by rodata objects**.
These are build-time fixups. The app's linked load sections contain resolved
pointers, **not an extra 1,751-entry runtime relocation table**. At four bytes per
pointer, some 7,004 bytes participate in those fixups, but many are essential
callbacks/array references. That total is not reclaimable headroom.

`param_desc_t` is already **16 bytes with no target padding**: one label pointer,
four one-byte range/format/default fields, a names pointer and unit pointer.
Its bounds were already narrowed. `TP` has 71 records / 86 pointer relocations;
`GP` has 59 / 93. `BOOT_TABLES` is already 16 bytes per record with no padding.
`sequence_starter_t` is already 8 bytes. `SEQUENCE_RHYTHMS` and the browser’s
kind/engine indices were already packed. These are not fresh savings.

There is residual tail padding in pages (12 vs 11 bytes), groove descriptors
(12 vs 10), and ROM synth-kit descriptors (44 vs 41). Across 47 pages, 24 grooves,
32 kits that is **191 gross data bytes**. Packing all three saved only **16 linked
bytes**, because access/index code grows on this target. Main/low/pool RAM stays
unchanged. `results/padding/` is useful rejection evidence; do not integrate this
batch or quote 191 as a target saving. Individual subcases were not measured.
The persistent editable drum-kit type was deliberately left unchanged.

The 12 engine records are 184 bytes each. Reordering fields may remove a few
padding bytes per engine, but packing callback-heavy hot engine descriptors
could trade alignment for instructions. No measured saving is claimed.

Unlinked LZ4 sizes of pointer-bearing tables are misleading: `TP` 323 bytes,
`GP` 314, `DRUM_GROOVES` 58 and the 46-entry kit-name table 11 all compress pointer
placeholders. Actual pointers differ after linking. A valid representation would
need explicit label/name indices, callback relocations, or startup reconstruction.
Do not sell these ratios as flash savings.

## Strings and identical data

The compiler’s mergeable string section is **10,017 bytes**, 1,331 complete unique
null-terminated strings, longest 28 bytes. Its ELF flags are SHF_ALLOC|SHF_MERGE|
SHF_STRINGS and alignment one. Exact literal pooling is already active; neither
alignment packing nor deleting repeated source spellings yields a corresponding
linked saving. This scan does not prove optimal suffix sharing.

LZ4 of that whole unlinked section is **8,123 bytes**: only 1,894 gross bytes of
payload saving for **10,017 persistent RAM bytes**, before pointer fixups/code.
That exceeds the baseline 9,176-byte main static gap; low RAM has 1,216 bytes.
It is not a viable drop-in boot packing candidate. `BP_NAMES` separately compresses
1,384 to 1,170 bytes: **214 gross**, at least 1,384 RAM plus relocation/init work.
Reject as a first choice beside the measured numeric-table trade.

Arrays whose names start `N_` total **2,348 bytes** in the baseline object. Using
16-bit string-pool offsets could theoretically halve that pointer storage
(**1,174 gross bytes**, not measured), but needs a generated stable pool, changed
descriptor types and name accessors throughout UI/help/protocol code. General XIP
offsets do not fit 16 bits in a 581-KiB image; only offsets inside a dedicated pool
would. A sentinel, optional units, dynamic/user names, suffix pooling and all raw
host paths need explicit handling. This is a bounded follow-up if larger savings
are required, not a proposed new string framework or a ready 1-KiB gain.

Exact whole-object linked byte comparison finds just one >=8-byte duplicate pair:
`N_ONOFF` and `N_ONOFF_E`, both 8-byte OFF/ON pointer arrays. Sharing is semantically
reasonable if no identity comparison depends on them, but at most 8 data bytes
before layout; no target patch is recommended for this alone. Different pointer
tables sharing some strings are not identical objects. Packed font metric blobs
are 11 bytes each but differ (8 vs 12); these are uniform-value representations,
not identical byte blobs.

`up_rec_none` is a retained 192-byte all-zero ROM fallback, not an unreferenced
sample/preset asset. Reusing unrelated zero RAM would require stable lifetime,
alignment and immutability proof and weaken its simple sentinel ownership. It
was not counted as free storage. `DS_KITS` LZ4 screens at 1,015 vs 1,408 bytes in
the unlinked object, but includes 64 unresolved name/style pointers. Its usable
compression would require pointer reconstruction and 1,408 RAM; no real saving
is claimed from that screening ratio.

## Retention and already-integrated work

The linked baseline has only the two-icon atlas's `ICON_ROWS` / `ICON_ROW_ID`,
66 payload bytes total. No old parameter-icon maps, full atlas, bitmap splash,
SLICE preset table or independent large-font bitmap is present in the retained
named objects. Raw factory preset arrays are RAM destinations restored by the
integrated sparse preset scheme, not an extra copy linked beside `BP_DATA`.
Boot descriptors keep their selected destinations/payloads intentionally alive.
Absence here means absent from the active target inventory, not a claim that
optional sources should be deleted.

Existing boot tables already cover 21,658 raw bytes with 7,696 payload bytes,
plus the separately packed UAC FIR. Font glyph deduplication, drum base dictionary,
field bit widths, preset sparsity, compact browser indices and mixed-width groove
masks are integrated and were not counted again. Original font glyph/large-font
sharing already avoids a separate large bitmap.

Prior `headroom-study/asset-dedup.json` shows 51 zones already share 40 unique
streams, avoiding 77,064 duplicated bytes; no remaining stream is a substring of
another and the optimistic boundary-overlap opportunity was only 37 bytes.
This audit confirms identical factory sample bytes in all candidates; it does
not resell that integrated sharing. Runtime sample decompression, lossy changes,
content deletion and partition enlargement remain excluded.

## Reproduction and remaining gates

From the repository in PowerShell, using only the private source/output root:

```powershell
wsl -d Ubuntu --cd /mnt/c/code/sloop-fm1/build/headroom-assets-51/source --exec env JIELI_TOOLCHAIN=/home/shaun/.cache/sloop-host-tools/jieli-linux-toolchains-20250324.1 FELUCCA_SLICE=0 python3 ../experiments.py baseline padding trim font-fixed zones extra-boot
wsl -d Ubuntu --cd /mnt/c/code/sloop-fm1/build/headroom-assets-51/source --exec python3 ../validate.py
wsl -d Ubuntu --cd /mnt/c/code/sloop-fm1/build/headroom-assets-51/source --exec python3 ../validate_extra.py
python build/headroom-assets-51/inventory.py
python build/headroom-assets-51/detail.py
python build/headroom-assets-51/asset_equivalence.py
```

Run sequentially: these scripts temporarily edit their **own** private snapshot
and restore it in `finally`. They must not be redirected to shared sources or
run concurrently against the same private source. New generated proof headers
may remain there, but baseline compile paths do not enable/reference them.
`source/build/` is the most recent experiment's output; frozen comparisons are
the named `results/` folders. No `tools/build_windows.py` command is needed against
the shared root. Native Zig may use its normal compiler cache, not shared
firmware outputs.

Validation evidence is under `validation/`: font baseline/candidate logs, trim
oracle and executable, zone initializer oracle/executable, and extra table
initializer oracle/executable. `asset-equivalence.json` records the identical
303,054-byte sample hash in all six target images. Startup metadata oracles are
also checked against original baseline ELF bytes. Hardware latency/watchdog,
stack high-water, complete audio goldens, and integrated UI tests remain gates
for a production integration; static budget success is not an audio-deadline
measurement.

**Recommendation:** integrate the fixed-metric font generator and comment-preserving
ragged trims first. If more flash is needed, extend the existing boot packer for
`GR_HANN`/`FM6_STATICS` before spending the larger RAM budget on zone metadata.
Preserve direct sample playback. Rebuild each final generator-backed combination
and its RAM/sections; do not sum these isolated prototype results.
