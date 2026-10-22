# Target compiler flags audit

Keep the production `-Oz` flags. The only measured size win was 128 bytes from
`-mllvm -inline-threshold=0`; it reproduced byte-for-byte, but leaves this frozen
integration snapshot 3,896 bytes over the real slot. That small gain does not
justify adopting an internal compiler option as a stability or fit solution.
No production files, shared outputs, generators, packages or Git metadata were
changed by this experiment.

## Frozen inputs and reproduction

The complete current `firmware`, `tools`, `tests` and existing `build/gen` were
copied once to `build/compiler-flags`. Subsequent parent changes are excluded.
The manifest is `build/compiler-flags/snapshot-sha256.json`; its SHA-256 is
`46a7bdf2f5835a12ab6b1ea2e5ac96b4cd924b76e3a7e4b3382b09c01d4008d8`.
`snapshot-verification.json` verifies the copied inputs after the matrix,
checking the original linker against the manifest. No assets were regenerated.

Compiler: JieLi `jieli-linux-toolchains-20250324.1`, reporting Clang 4.0.1,
invoked directly under WSL Ubuntu with `-target pi32v2`. Toolchain executable
hashes are in `toolchain-sha256.txt`; compiler version is in `compiler.txt`.
The driver is `build/compiler-flags/run.py`, derived from the existing
`build/piano-space-audit/run.py` workflow. From the private directory:

```sh
python3 run.py                 # matrix, including baseline repeat
python3 run.py inline-0        # additional independent winner repeat
python3 run.py lto-wrapper     # installed toolchain, correct LTO link path
python3 extract_toolchain.py   # safely extract downloaded official archive
python3 run.py new-toolchain   # private 20260730.1 default and LTO builds
python3 summarize.py          # saved budget deltas and input verification
```

The private `firmware/app.ld` changes **only** XIP LENGTH from `0x8DFBC`
(581,564 bytes) to `0x180000`, identically for every variant. Its original is
`original-app.ld`. This allows measurement of overflowing variants; these are
**not flashable artifacts**. `build.check` retains the real app-slot limit and
reports every successfully linked image as oversized. All other linker regions,
addresses, runtime code and configuration remain fixed.

Every C compile uses the optimization flags below plus:

```text
-target pi32v2 -ffunction-sections -fno-builtin -Wall -Wno-unused-function
-DFELUCCA_BOOT_TABLES=1 -Ifirmware/hal -Ifirmware/src -Ibuild/gen
-DFELUCCA_SLICE=0 -DFELUCCA_ID="FM-1_900"
```

All other `FELUCCA_*` build overrides are unset, and VERSION is unset; source
defaults apply. Assembly uses `-target pi32v2 -c`. Linking uses the unchanged
`pi32v2/bin/ld -T firmware/app.ld` and the standard five objects, without LTO or
new runtime libraries. Exact full argument arrays for compilation, linking and
extraction are retained per variant in `results/<name>/commands.json`.

## Measurements

Sizes are target linked bytes, not host object estimates. `.text` includes
read-only constants as defined by this linker. Image also includes the RAM-code
and initialized-data load images plus alignment.

| Variant / optimization flags | Image B | Change vs -Oz B | .text B | .data B | Target budgets |
| --- | ---: | ---: | ---: | ---: | --- |
| `-Oz` | 585588 | 0 | 582048 | 524 | pass |
| `-Os` | 591952 | +6364 | 588400 | 528 | pass |
| `-O2` | link failed | — | — | — | unavailable |
| `-Oz -fno-inline-functions` | 586744 | +1156 | 583200 | 528 | pass; outlined work caveat |
| `-Oz -mllvm -inline-threshold=25` | 588388 | +2800 | 584848 | 524 | pass |
| `-Oz -mllvm -inline-threshold=0` | 585460 | **-128** | 581920 | 524 | pass |
| `-Oz -fno-jump-tables` | 586228 | +640 | 582688 | 524 | **fail** |
| `-Oz -fno-unroll-loops` | 585588 | 0 | 582048 | 524 | pass |
| `-Oz -fno-vectorize -fno-slp-vectorize` | 585588 | 0 | 582048 | 524 | pass |
| `-Oz -mllvm -enable-merge-functions` | compiler rejected | — | — | — | unavailable |
| `-Oz -flto` with plain `ld` | linker rejected | — | — | — | unavailable |
| `-Oz -flto` with vendor `lto-wrapper` | 585556 | -32 | 582044 | 496 | pass |

Baseline repeat has the same 585,588-byte image and SHA-256
`13dd8062cad798400faaee18384d56b59dbc4e4c259eda29ea5661ea2d532fff`.
Threshold-zero repeat has the same 585,460-byte image and SHA-256
`5ae83a5768afac5304d965c04b50d18536d5141857203e3c78f16254668cc44c`.
Disabling vectorization is also byte-identical to baseline. Disabling unrolling
has equal size but a different image hash; equal size alone is not equivalence.

`-O2` emits an unresolved `__udivdi3` call in `px_modal_exciter`. No runtime was
added to hide this failure. Function merging is an unknown LLVM option in this
compiler. `-flto` compilation produces an object the existing target linker
rejects as an unrecognized format. That initial experiment used plain `ld`,
which is the repository's current linker invocation. The follow-up below
corrects the broader implication: the installed toolchain **does support LTO**
when invoked through its vendor wrapper.

## RAM, layout and DSP gates

All initially linked non-LTO variants use 85,608 bytes of the 98,304-byte ordinary RAM region
(12,696 spare), `.bss` 85,080 bytes, `.lowram` 22,772 bytes, pool 332,876 of
344,064 bytes (11,188 spare, exceeding the required 8 KiB), `.c1_text` 60 bytes,
and `.noinit` 15,464 bytes. `.ram_text` is 2,956 bytes except `-Os` at 2,964.
Low-RAM spare after code and buffers is 6,724 bytes, or 6,716 for `-Os`.
The fixed stack/guard addresses remain unchanged. These are static layout
checks, not stack high-water measurements.

Every linked variant passes entry-stub/address checks, scratch containment,
ROM-reference checks and the `.ram_text` invariant: 925 instructions, **no
calls**. Every linked variant fails only the real image-size check in
`build.check`. The private linker still enforces all non-XIP region limits.

`tests/target_budget.py` ran against every resulting target disassembly and the
frozen `tests/target_budget.txt`, without updating budgets. Full per-function
instruction/loop/divide/call/weighted-cost data are in each `budgets.json` and
`budget-check.txt`; all changes versus baseline are in `budget-deltas.json`.

Threshold zero leaves every monitored loop instruction, divide, call and cost
count identical to baseline. Only the ISR's total instruction count changes
from 232 to 231; its loop remains 88 instructions, zero divides, three calls,
cost 256. Thus there is no measured render-loop CPU regression for the 128-byte
win, but no hardware deadline or complete-program speed claim follows.

Disabling jump tables fails `digital_render`: 369 loop instructions/cost versus
the recorded budget 333 (+10.8%, over the +10% allowance); baseline is 339.
It also increases image size, so it is rejected on both grounds.

Reduced inlining can make monitored functions look dramatically cheaper merely
by moving their work into callees. For example `-fno-inline-functions` reduces
`grain_render`'s local cost to 49 and ISR cost to 5; the checker does not charge
the transitive callees. Do not interpret these budget passes as audio CPU wins.
`-Os` also changes loop structure substantially and gains no size benefit.

## Decision and limits

No broad compiler setting tested supplies the required several kilobytes.
Prefer focused source reductions with existing behavior tests, retaining the
upstream-compatible production `-Oz` configuration. Threshold zero is a small,
reproducible candidate only if a later release needs the final 128 bytes and
receives target behavior and timing validation against its own frozen baseline.

No fast-math, unsafe floating-point options, ABI changes, short enums, packing,
signed-overflow assumptions or altered aliasing semantics were introduced.
Those are outside this audit's compatibility scope. No source algorithm changed,
so host audio goldens were not rerun; host builds would not validate these
target flag transformations anyway. Target execution equivalence, audible
output, worst-case latency, cache effects and physical hardware deadlines remain
unverified. Internal LLVM flags are compiler-version-specific and carry a
maintenance cost. No flags were adopted and no artifacts were packaged.

## Follow-up: correct LTO invocation and newer toolchain

The prior statement that LTO was unsupported by the tested build path described
only the plain-`ld` experiment; it was insufficient as a toolchain capability
assessment. The installed 20250324.1 distribution already includes
`pi32v2/bin/lto-wrapper`, a Python wrapper that invokes `ld --plugin LLVMgold.so`.
[JieLi's SDK build documentation](https://doc.zh-jieli.com/SDKWiki/master/Jieli-Tech/fw-AC63_BT_SDK/2-getting-started/2.2-build-system.html)
explicitly selects this wrapper for LTO builds. Its example targets a different
SDK/chip; the frozen local build supplies the evidence for this firmware.

With unchanged source, target, compile flags and linker script, adding `-flto`
and replacing only the linker executable with `pi32v2/bin/lto-wrapper` succeeds.
The 585,556-byte image is 32 bytes smaller than baseline but still 3,992 bytes
over the real slot. `.text` is 582,044 bytes; `.data` 496; `.bss` 85,096;
ordinary static RAM 85,592; `.noinit` 15,436. `.ram_text` remains 2,956 bytes,
925 instructions with no calls; `.c1_text`, lowram and pool remain unchanged.
All target budgets pass and all checks except slot size pass. Results and
commands are under `build/compiler-flags/results/lto-wrapper`; raw log is
`lto-wrapper.log`. This is a static build check, not proof of target runtime
equivalence or safe flashing.

The official download endpoint
[JieLi Linux toolchain](https://pkgman.jieliapp.com/s/linux-toolchain) returned
an XZ archive containing `jieli-linux-toolchains-20260730.1`, rather than the
previously reported 20250805.1. The archive SHA-256 is
`a1545cddbc451a06bac43eb5cd96d831470c42a97c5c0b1f98879a20fc7647bd`.
It is stored and extracted only under `build/compiler-flags`, leaving the
installed 20250324.1 toolchain unchanged. Safe archive extraction uses Python's
`filter='data'`. No SDK CPU flags, runtime libraries or ABI settings are copied
from the documentation example.

The newer distribution also reports Clang 4.0.1. Its compiler and LLVMgold
plugin hashes differ from the installed distribution, while the `pi32v2` linker
hash is identical. Full hashes are in `new-toolchain-sha256.txt`. Therefore this
is a distinct vendor distribution, not evidence of migration to a modern LLVM
major version.

| Frozen snapshot build | 20250324.1 image B | 20260730.1 image B | Comparison |
| --- | ---: | ---: | --- |
| `-Oz`, plain linker | 585588 | 585588 | byte-identical image |
| `-Oz -flto`, vendor wrapper | 585556 | 585556 | byte-identical image |

The newer-toolchain image hashes match the respective old-toolchain hashes,
including LTO hash
`da9d7eed2eb8ebafa796db7749b79ed71fa221ab216b9fcb40c703ccc9ad24f6`.
RAM section sizes, target budgets and build-check outcomes also match each
corresponding build. Both retain the image overflow, so the newer distribution
does not improve fit for these tested configurations. Results are in
`results/new-oz` and `results/new-lto-wrapper`, with `new-toolchain.log` and
`new-compiler.txt` alongside the driver. Final manifest verification still
reports all 280 frozen inputs unchanged.

Updating the toolchain is technically feasible as a separate build-tooling
change, but these measurements supply no size reason to do so. The current
toolchain already supports LTO through its correct wrapper. No global install,
production linker change, flashing or packaging was performed.
