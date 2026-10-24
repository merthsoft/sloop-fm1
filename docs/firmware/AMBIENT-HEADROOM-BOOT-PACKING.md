# Ambient headroom: startup-only data packing

2026-10-24. Audit only, frozen from the actual dirty 2.8.0.58 working files.
Production sources, shared generated assets, shared build outputs, installer,
version and Git state were not changed by this audit. All experiments are under
`build/headroom-ambient/boot-packing/`. Subsequent parent commits do not redefine
this baseline.

**Measured recommendation:** a focused combination saves **880 application
bytes for 1,008 additional main-RAM bytes**. A more aggressive combination saves
**1,728 application bytes for 4,000 additional main-RAM bytes**. Neither adds
decoding during playback or rendering. Whole-factory-sample expansion is rejected
on resident-memory capacity, regardless of its compression ratio.

## Ranked target results

Every row is a complete same-source/version target link against the unchanged
584,336-byte slot. Savings are relative to the 583,388-byte baseline. Separate
rows must not be added; the named combinations were actually linked.

| Candidate / result directory | Application B | Flash saved B | Main static RAM B | Main RAM delta B |
|---|---:|---:|---:|---:|
| `baseline` | 583,388 | — | 86,176 | — |
| **1. `focused`: numeric pair + zero sentinel + preset field bits** | **582,508** | **880** | **87,184** | **+1,008** |
| **2. `best`: focused + zone metadata + six-bit preset names** | **581,660** | **1,728** | **90,176** | **+4,000** |
| `extra`: GR_HANN / FM6_STATICS | 582,940 | 448 | 86,992 | +816 |
| `zones`: sample-zone metadata only | 582,844 | 544 | 87,600 | +1,424 |
| `names6`: six-bit preset names | 583,068 | 320 | 87,728 | +1,552 |
| `zero`: dedicated empty preset sentinel in BSS | 583,100 | 288 | 86,368 | +192 |
| `preset-bits`: replace sparse preset field stream | 583,260 | 128 | 86,176 | 0 |
| `names`: ordinary LZ4 preset names, alternative | 583,164 | 224 | 87,728 | +1,552 |
| `combined`: numeric pair + zones + LZ4 names | 582,172 | 1,216 | 89,984 | +3,808 |
| **Reject `expanded`: eight numerical tables** | **583,724** | **−336** | **88,016** | **+1,840** |
| Reject `all`: expanded + zones + six-bit names + zero sentinel | 582,588 | 800 | 91,184 | +5,008 |

Every row retains **28,928/29,496 B low RAM**, **332,876/344,064 B pool**, the
**8,192 B pool reserve**, 6,216 B persistent main-loop scratch, and the existing
stack/guard/flash regions. No destination is allocated in low RAM or the pool.

The focused combination leaves **1,828 B flash / 11,120 B main RAM** free. The
larger combination leaves **2,676 B flash / 8,128 B main RAM** free. Low RAM
remains **568 B free** in both. These are linker margins, not measured stack
high-water marks or permission to spend audio reserves. For ambient/drone work,
start with the focused option if the new engine needs persistent main RAM;
choose the larger option only after budgeting that engine's resident state.

In the larger combination `.text` falls by 1,728 B and `.bss` grows by 4,000 B.
`.data`, `.ram_text`, `.c1_text`, `.lowram`, `.pool` and `.noinit` are unchanged.
Raw destination sizes and linked deltas differ because of alignment/global
layout. The 288 B sentinel result includes code/layout effects beyond its
192-byte data removal; it is not a claim that the sentinel itself was 288 B.

## Baseline, inputs and reproducibility

Read the architectural-headroom, asset, link, storage, UI, redundancy, compiler
and optimization reports before choosing candidates. Scalar font metrics,
ragged trims, sparse preset restoration, punch initialization, direct legacy
conversion, compact icons, mixed-width groove masks and factory sample sharing
are already integrated. Their historical gains are not counted again. The
numeric pair and zone metadata below were previously investigated but remain
unintegrated; their figures here are **fresh .58 measurements**, not new ideas
or copies of old linked deltas.

`manifest.json` hashes 551 frozen firmware/tool/test/asset/generated inputs.
`original/` retains the 175 firmware/tool/generated inputs before experimental
edits; `input-verification.json` confirms all 175 match the freeze. A later
shared-input comparison found only concurrent parent changes to
`tests/pages_deployment_test.py` and `tools/web_provenance.py`; all firmware and
shared generated inputs still match. The audit never writes those shared files.

Baseline application SHA-256:
`f8babc2430d20813c5a4e467a946b3c2d100757d42b8d54dc2f102ff41ee0e57`.
Larger candidate SHA-256:
`487a489f7eebba6e504a1ca1aae415771914e5c6dc3d7b925d1eb3c8e1394f1b`.

Tools: WSL Ubuntu; installed JieLi
`/home/shaun/.cache/sloop-host-tools/jieli-linux-toolchains-20250324.1`,
Clang 4.0.1, `-target pi32v2`. Normal `build_app()` flags:
`-Oz -ffunction-sections -fno-builtin -Wall -Wno-unused-function`, normal
firmware/hal/src/gen include paths, boot tables/presets enabled,
`FELUCCA_SLICE=0`, normal `FM-1_900` build identity and source-default .58
version. No link-region enlargement, compiler experiment, loader, SDK package
or installer build is involved. The only compiler warning is the pre-existing
missing-braces warning in `note_chain_actions.c`.

`run.py` imports the build module from the **private** tree and invokes only
`build_app()` and `check()`. Its `SRC`, `OUT` and `GEN` all resolve inside the
audit directory. Normal preset probes/generation therefore also run privately.
Each result retains ELF, object, probe, application, disassembly, symbols,
sections, checks and validation logs. `measurements.json` collects the sizes;
`inventory.json`, `screen.json` and `codec-screen.json` retain screening evidence.

Reproduce sequentially, never concurrently against the same private tree:

```powershell
wsl -d Ubuntu --cd /mnt/c/code/sloop-fm1/build/headroom-ambient/boot-packing --exec python3 prepare.py best
wsl -d Ubuntu --cd /mnt/c/code/sloop-fm1/build/headroom-ambient/boot-packing --exec python3 run.py best
wsl -d Ubuntu --cd /mnt/c/code/sloop-fm1/build/headroom-ambient/boot-packing --exec python3 validate.py best
```

Use `focused`, `extra`, `zones`, `names`, `names6`, `zero`, `preset-bits`,
`combined`, `expanded` or `all` instead of `best` for the other rows.
`prepare.py baseline` restores original inputs before a baseline build.
The private working tree currently contains the focused experiment; authoritative
comparisons are the named `results/` directories. No generated experiment header
should be copied blindly into production.

## Existing startup packing and decoder reuse

The generated boot list already expands **20 tables: 21,210 raw B from 7,674
payload B**, plus UAC FIR's **1,920 raw B from 351 payload B**. It covers sine,
pitch, timing/envelope/filter/level/FM6 numerical tables, small-font data/offsets,
drum bases and FM6 factory patches. Existing direct residual bitpacking and
orders 0–4 finite differences are often much better than generic LZ4 for smooth
tables. Font glyph and drum-base data use ordinary LZ4. FIR has its existing
specialized column/second-difference decoder.

The 171 current factory presets already restore **6,156 B** of low-RAM records.
Their sparse stream is **3,019 B**, stored in **2,813 B LZ4**. Restoration uses
the existing `proj_tmp`, then constructs name pointers into the **1,542 B ROM
name stream**. There is no second raw factory-record copy in flash.

`boot_table_unpack` is **636 target bytes in baseline and every experiment**.
All accepted packing candidates reuse it; incremental generic decoder text is
**zero**. Net linked figures above include added descriptors, startup code and
layout effects. A target boot descriptor is 16 B; native host descriptors are
larger and are not used for flash accounting.

Startup order is: interrupts disabled; BSS/pool/low RAM initialized; data copied;
punch state and RAM code initialized; boot tables; boot presets; mailbox/guards;
recovery or normal main; later core 1/audio/IRQ enable. The candidates initialize
their destinations during that same early interval. Packed flash is never read
by a new playback/render decoder, and the restored destinations remain resident
for the entire run, including recovery.

## Candidate details and implementation handoff

### Numeric pair: 448 B flash for 816 B linked RAM

`GR_HANN[257]`: 514 raw B → 106 packed B, third differences/3-bit residuals.
`FM6_STATICS[77]`: 308 raw B → 189 packed B, first differences/19-bit residuals.
The existing packer already supports both encodings. Two target descriptors cost
32 B; final linked saving is 448 B rather than the 527 B payload difference.
Expansion writes each final array directly with scalar decoder state; no
whole-table scratch is needed. All 822 bytes match the baseline ELF.

Integration: teach a deterministic numeric-table generator to emit these
source-defined tables, retain raw definitions when boot packing is disabled,
and include them in the ordinary boot list. The prototype moves them to
`audit_extra.h` and uses a second list purely for isolation; consolidate into
the existing generated list and remeasure. Ensure host boot-table-enabled
harnesses also initialize the new tables, or use their independently generated
raw headers. Upstream burden: low to moderate, localized to two table definitions,
generation and boot-list ownership; no sound values or public formats change.

### Dedicated zero sentinel: 288 B flash for 192 B RAM

`up_rec_none` is a required, 192-byte all-zero empty-slot fallback. It is not dead
code. Replacing `static const up_rec_t up_rec_none` with a dedicated zeroed
`static up_rec_t up_rec_none` moves it from ROM to BSS. It does **not** alias an
unrelated scratch buffer, use a decoder, allocate new boot scratch or depend on
another object's lifetime. `up_rec()` still exposes a const pointer; the only
references are its declaration and that returned address. Target symbols confirm
the same 192-byte object in `.bss`, with normal BSS initialization preceding users.

Upstream burden: very low, one declaration. Keep it private and never add a write
through its address. The semantic cost is losing physical ROM immutability for a
small dedicated sentinel, rather than losing the empty-slot behavior.

### Preset fields: 128 B flash, no RAM increase

The alternative generator stores each table's 32 columns using that column's
maximum byte value to choose 0–8 bits. Each column has a width byte; values are
bitpacked with byte alignment at column boundaries. The resulting stream is
**2,893 B**, LZ4 **2,625 B**, compared with sparse 3,019 / 2,813 B. The boot-only
reconstruction loop offsets some payload savings: final target benefit is 128 B.

This is an **alternative** to sparse field restoration, not a second copy.
The same 6,156 B preset destinations and name pointers remain. Required
`proj_tmp` capacity falls **3,019 → 2,893 B**; there is no extra resident buffer.
Width zero produces zeros without reading a payload byte; widths through eight
preserve all 32 bytes per record, including padding and signed-field bit patterns.

Upstream burden: moderate, `pack_presets.py` generation and its boot restore
template change. Keep probe-based ABI/name extraction, input bounds and raw host
support. Extend existing boot-preset tests to both formats; no user project,
protocol or runtime preset-selection format changes are needed.

### Zone metadata: 544 B flash for 1,424 B linked RAM

The 51 `smp_zone_t` records occupy **1,428 B**, have a 28-byte target ABI and no
relocations. Existing LZ4 packs them to **854 B**. The final target saving is
544 B, freshly measured here (the older audit's 528 B is not reused).
Startup writes directly to the final main-RAM array; no new scratch exists.
All 1,428 bytes, including loops, rates, key ranges and state, match baseline.

Integration: use a relocation-checked raw **target object probe**, as the existing
preset generator does; derive row count and ABI rather than hard-coding 51/28.
Teach `gen_samples.py` raw/boot destination emission and give this init its own
guard. The prototype's explicit `FELUCCA_BOOT_ZONES` path calls it before presets.
Do not extract addresses from a linked image to construct production assets.
Upstream burden: moderate, sample generator/probe/build/startup coordination.
There is no audio-byte change or new sample decoder.

### Preset names: 320 B flash for 1,552 B linked RAM

Current names contain NUL, space, hyphen, digits and uppercase ASCII. Six-bit
`character & 63` values are injective for this alphabet: 1–26 map back to A–Z;
space/digits/hyphen retain their values and zero remains NUL. The payload is
**1,161 B**, including the existing residual format's four-byte zero base.
An ordinary boot descriptor (`unit=1, order=0, bits=6`) reuses the existing bit
decoder, followed by one ASCII-restoration pass. The destination preserves the
complete 1,542-byte stream, including the final C-string padding NUL.
No dictionary or scratch buffer is needed. Preset name pointers are assigned
only after restoration. Ordinary LZ4 is the simpler alternative: 1,285 B payload,
224 B linked saving for the same RAM.

Integration: implement codec selection inside `pack_presets.py`. Validate the
alphabet; **fall back to LZ4/raw for unsupported future names**, including `@`
and lowercase, rather than silently truncating them or restricting upstream
preset naming. The experimental generator deliberately asserts its frozen
alphabet and needs that fallback before production use. Upstream burden:
moderate and localized to generated factory names; no dynamic/user strings or
render accessors change. This is lower flash-per-RAM efficiency than the focused
combination and should be optional when reserving ambient state.

## Inventory, codec screens and rejected work

`inventory.py` scans retained named objects in the baseline object/ELF. It records
relocations explicitly and round-trips LZ4, zlib, XZ and byte RLE. `screen.py`
tries finite differences/bitpacking across integer units; those are **payload
screens**, not linked savings. `codec-screen.py` independently checks preset
transpose, dictionary and six-bit-name round trips.

| Asset / raw B | LZ4 B | zlib B | XZ B | Interpretation |
|---|---:|---:|---:|---|
| Factory samples / 303,054 | 285,248 | 238,515 | 212,172 | Resident RAM cannot fit; reject boot expansion |
| Sample zones / 1,428 | 854 | 620 | 580 | LZ4 reuse has real linked evidence above |
| Preset names / 1,542 | 1,285 | 801 | 948 | Six-bit reuse beats ordinary LZ4 without a new decoder |
| Mergeable literal section / 10,046 | 8,163 | 5,418 | 5,216 | Pointer remapping/decoder costs unmeasured |
| Already-packed BP_DATA / 2,813 | 2,825 | 2,347 | 2,488 | Not a second LZ4 opportunity |
| Already-packed drum bases / 2,831 | 2,836 | 2,559 | 2,648 | New entropy decoder needed for further savings |

zlib/XZ numbers include their stream wrappers but **exclude an embedded decoder,
dictionary/scratch and integration code**. No target decoder for either was
added or linked, so none of those ratios is a net firmware saving. A future
entropy-codec investigation should beat the measured reuse options after linking
its decoder and proving working-memory bounds; this audit does not recommend
switching codecs from these ratios alone. RLE is generally poor (e.g. samples
575,070 B) and the dedicated zero sentinel is better represented by BSS.

* **Expanded numerical batch rejected:** adding IMA_STEP, FM6_LFO_DELTA,
  FM6_INIT, PH_A, FM6_PENV_TAB and FM6_PENV_RATE to the numeric pair reduces
  1,794 raw B to 806 payload B, but **grows the image by 336 B** and consumes
  1,840 main-RAM B. Descriptors and changed constant/global access/code layout
  dominate. For example, emitted `mix_block` grows 316 B and `usb_poll` 106 B;
  these are symbol-size observations, not measured cycle costs. The batch passes
  target budgets but is inferior headroom. Its individual added tables were not
  separately linked, so do not quote their 40–123 B payload gains as net wins.
  `all` confirms that including this batch also weakens a larger combination.
* **Preset transpose/dictionary screens rejected:** all fields row-major LZ4
  3,421 B; global column-major 2,931 B; per-column value dictionaries 3,139 B
  stream / 2,891 B LZ4. These are worse than the existing 2,813 B sparse payload,
  before new decoder costs. Per-table field bit widths are the measured useful
  alternative above.
* **Sequences/grooves:** starters occupy 192 B with 24 relocations; groove
  descriptors 288 B with 48. Their LZ4 screens (153 and 58 B) include unresolved
  pointer placeholders. Mixed-width groove masks, compact starter degrees and
  packed rhythm/browser metadata are already present. A valid boot representation
  needs label/mask pointer reconstruction plus resident arrays and descriptors;
  no fresh net saving is established. Do not undo the prior mixed-width design.
* **Labels/control descriptors:** TP 1,136 B / 86 relocations, GP 944 B / 93,
  DS_KITS 1,408 B / 64, pages 564 B / 47. Their object compression ratios are not
  valid runtime images. Stable label/callback indices could enable reconstruction,
  but that is a broader descriptor-format project. Ordinary literal pooling is
  already enabled. Copying the 10,046 B literal pool would leave only 2,082 B of
  the baseline main-RAM gap before pointer-remapping state and decoder overhead;
  it cannot coexist with the measured larger option and retain its RAM margin.
* **Wave/audio data:** existing sine/drum/FM6 boot assets are already packed and
  ordinary sample streams remain direct XIP. No per-hit, per-sample, cache-miss
  or render-time decompression is proposed. No font-only experiment is included.

### Whole-factory-sample resident-capacity rejection

Even preserving the current encoded sample format, boot expansion requires a
**303,054-byte resident destination**. Main RAM has **12,128 B free**; low RAM has
568 B; pool has 11,188 B apparent gap, of which 8,192 B is reserved. Main RAM
alone is short **290,926 B**. Even incorrectly treating all three disjoint
unreserved gaps as one space gives only **15,692 B**, still short **287,362 B**.
Existing active audio canvases/buffers, scratch with later owners and reserves
are not available destinations. Decoder scratch and alignment would worsen this.
Compression improves flash storage, not the required resident bytes. Therefore
no sample-expansion prototype or playback change was made.

## Startup scratch, ordering and latency

All new numerical, zone and name output arrays are persistent **main BSS**.
They do not borrow low RAM, active audio buffers or `proj_tmp`. Numeric/zone
decoding writes final output directly. Six-bit names write their final buffer,
then map ASCII in place. The zero sentinel uses normal BSS zeroing. Preset
fields reuse the existing `proj_tmp` for one stream, never concurrently with
storage/project operations; the focused/larger variants reduce this scratch
requirement to 2,893 B. Additional decoder state is scalar locals and a small
automatic descriptor, not a second whole-table allocation. Hardware stack peak
was not measured; the permanent stack regions were unchanged.

No host timing is used as a target timing. Useful work counts for the larger
candidate are **2,206 numerical residual-bit iterations**, **9,252 name-bit
iterations**, 1,542 ASCII fixups, 1,428 zone output bytes, and 5,472 preset-field
stores (19,480 packed field bits processed by an accumulator, not a per-bit C
loop). A transparent sizing scenario assigning 100 target cycles to each
residual/name-bit iteration and each of the 9,264 numerical/name/zone/field
output items gives about **2.07 million cycles / 5.8 ms at the documented
360 MHz**. At 50–500 cycles per such work item it is roughly 2.9–28.8 ms.
This is a deliberately rough planning estimate for the candidate restore work,
not measured additional boot time, a cycle bound or a watchdog guarantee; some
preset work replaces existing startup work, and flash wait states/other boot
costs are not modeled. Measure actual cold/recovery boot and watchdog margin
on hardware before integration acceptance.

## Correctness evidence and remaining gates

Native C validation uses installed Zig 0.13.0, `-O2 -fsanitize=undefined`, with
the actual generated initializer/decoder code. Final `best` and `focused`
tests poison preset destinations before each of three restorations, validate
all **171 names and 5,472 field bytes**, reject zero scratch capacity, and
check scratch boundary canaries. Packed names are also poisoned before restore.
Numerical and zone destinations are independently poisoned and compared against
raw bytes from the **baseline linked ELF**, not only generator oracles.

Stable SHA-256s:

* Preset fields: `3e9cb1a964cd16315637a765014ec0dd126ccbdc6ba4e16e7147143a10e20411`.
* Semantic preset names: `4e399d38df086c64dfa0e3f160df9bf204fcafd793128d448ff9e41a07499e15`.
* Full 1,542-byte padded name stream: `f3dc0c7d9813ca7071c9b13fadf46eb30663a98e50a2a653d39ff5e3f5754e81`.
* All 303,054 unchanged sample bytes:
  `dcad9fe8f8c08a796d1b433c5cc0f96af46416502833b256593994fe7cccfe17`.

Each measured candidate passes `build.check`, all 24 target DSP budgets and
hot-helper inlining caps. RAM flash code remains 925 instructions with no calls;
core-1 waiting code remains 60 B. The larger candidate has no increased reported
DSP function instruction/loop/call/divide counts; grain's static weighted loop
cost changes 1,756 → 1,747 with unchanged instruction/call counts. This is static
disassembly evidence, not a physical speedup or deadline claim.

Concrete next step: integrate either **focused** or **best**, using its retained
`candidate.patch` as a review aid and the generator/host/fallback corrections
above as required work. Regenerate all assets from scratch, rebuild the final
combined source, and rerun actual boot-preset/raw-host parity, complete audio
goldens, sample/grain/metadata selection tests and UI/protocol name tests.
Then measure cold/recovery/watchdog boot and stack high-water on hardware.
Full application audio/UI golden suites and physical tests were **not** run by
this isolated representation audit. No version bump or publishing is authorized
or necessary for this handoff.
