# .91 UI flash audit

2026-10-28. **Recommended review candidate: 1,488 B net flash saved, 0 B static RAM growth.** The complete private candidate links at **582,808 / 584,336 B**, leaving **1,528 B**. Production source and Git state were not changed. This is a measured cold-UI call-sharing candidate, not a demonstrated device speed improvement or a completed multi-KiB view-model rewrite.

## Baseline and scope

The frozen **dirty .91** snapshot, including hold-delay and worker-hardening changes, was copied from `build/flash-audit-91/baseline/source/`, never exported from HEAD. Its private reproduction is exactly **584,296 B**, main **97,500 / 98,304**, low data **28,928**, pool **335,424 / 344,064**. Main free is 804 B; low free is 464 B after worker code/alignment; pool free is 8,640 B, including the **8,192 B retained reserve**, leaving 448 B beyond it. Every candidate below retains those extents and reserves. `.noinit` is also unchanged at 15,472 B.

All experiment sources, compiler/generator outputs, packages, tests and logs are under **`build/flash-audit-91/ui/`**. Only this new report is outside that directory. The frozen manifest recheck covers **346 files, zero mismatches** (`ui/frozen-check.log`). Generated headers and pinned SDK bytes came from the supplied snapshot. Every target invocation explicitly used `FELUCCA_AUDIO_DIAG=0`, the pinned WSL Ubuntu JieLi toolchain and the frozen `build/build-target.py`; target builds were sequential within this chat. No live flash, installer changes, shared build outputs, version changes, commits, PRs or delegation occurred. The supplied build entry point also creates loader/package artifacts, all privately; none was installed or published.

Read the architectural, engine integration, comparison, literal-storage, optimization and earlier UI/duplicate-code audits before selecting changes. The prior 496 B obsolete TRACKS-column deletion, shared parameter-card helper, compressed literals/preset metadata, shared modulation model and .90 engine optimizations are **already implemented**, not new savings here. Historical 4–10 KiB UI estimates remain planning estimates. No sample packing, reduced quality, reduced overload protection or additional restored-literal RAM is proposed.

## Ranked measurements

Sizes are actual final linked application bytes, including helpers, literal transform and alignment. All rows use the same .91 version/flags and main/low/pool **97,500 / 28,928 / 335,424 B**.

| Rank / variant | Image B | Saved vs baseline B | Flash free B | Disposition |
| --- | ---: | ---: | ---: | --- |
| Baseline | 584,296 | — | 40 | Exact reproduction |
| 1. Full four-slice combination | **582,808** | **1,488** | **1,528** | Recommended for review, subject to hardware UI/stack qualification |
| 2. Small-font calls alone | 583,496 | 800 | 840 | Narrow fallback, one helper |
| 3. Fixed palette text calls alone | 583,624 | 672 | 712 | Alternative standalone slice; overlaps font sharing |
| 4. Fixed-palette rectangles/lines alone | 584,072 | 224 | 264 | Small standalone gain |
| 5. Full-width band setup/blit alone | 584,088 | 208 | 248 | Small standalone gain, tail branches |
| Font + palette text | 583,288 | 1,008 | 1,048 | Measured intermediate |
| Font + palette + bands | 583,048 | 1,248 | 1,288 | Measured intermediate |

**Do not sum standalone savings.** Their sum is 1,904 B, while the actual complete combination saves 1,488 B. Font/palette substitutions overlap, and register allocation changes across combinations. The incremental 208/240/240 B in the ordered combination are not independent savings claims. Standalone non-font builds were linked independently by `ui/independent.py`; their outputs/logs are in `ui/results/independent-{palette,bands,primitives}/`. They are measurement alternatives, not separately UI-tested integration recommendations. The full combination and font+palette intermediate received the UI suite.

## Exact patches and application order

The frozen tree mixes LF and CRLF. In a **private review copy**, normalize C source line endings to LF before applying these LF patches; otherwise GNU patch rejects some contexts. `ui/apply-ordered.sh` performs that normalization and applies the four patches when run from a sibling candidate source directory. Its result was independently checked against the measured final C files (`patch-check-lf.log`). Never run it in the immutable snapshot or production tree during this audit.

Against that review copy, choose **one** route:

1. Apply **`build/flash-audit-91/ui/final-combined.patch`** once; or
2. Apply **`01-text.patch`**, **`02-palette.patch`**, **`03-bands.patch`**, **`04-primitives.patch`**, in that order, all beneath `build/flash-audit-91/ui/`.

These routes produce the same C source. Do not apply the combined patch after the ordered patches. Patches contain only `firmware/src/gfx.c` and changed UI C call sites. Tests/build harnesses are private evidence, not production patch content. Full candidate source is **`ui/primitives-source/`**. The plain-font, font+palette and band intermediate sources are retained as `text-firmware/`, `palette-firmware/` and `bands-source/` respectively. `evidence.py` generated the ordered patches; `combined-patch.py` generated the combined patch. Review and preserve these ignored artifacts before cleaning build storage.

Baseline application SHA-256: `c98a63741a9f16f2b9f7faa753dd31c980241a7013e56121339fbb03274552ad`.

Final application SHA-256: `3933b791e05d844c2268fae2b4c5e2e2ce5147120ee77ca40dd4d9d75f2be41e`.

## What changes, and why it saves bytes

**Small-font text:** 149 existing `cv_text(x,y,&FONT_S,text,color)` sites become four-argument `cv_text_s` calls. One 24 B target wrapper supplies the fixed font and calls the unchanged rasterizer. Font/palette specialization then replaces 102 of those sites with three-argument wrappers for the seven frequently used existing colors. Each wrapper reads the current palette/NEON gray at execution time; colors are not frozen at startup. Target helper bodies are 30–32 B each. Variable colors and font choices retain their existing arguments. All text return values remain live where used for chained layout.

**Bands:** 76 full-width black canvas setup/zero-x blit sites share `cv_band(h)` and `cv_band_blit(y)`. Width, height, background, canvas y offset, draw ordering, dirty decisions and blit coordinates stay the same. Target helpers are 18 and 8 B and tail-branch to the original implementation. No display command interpreter, framebuffer allocation, heap or scene cache is added.

**Rectangles/lines:** common fixed palette arguments move from repeated call setup into seven rectangle and five line wrappers, each 22–28 B. Geometry and ordering remain in the owning screen. The candidate preserves palette lookup semantics, clipping and the existing implementation of each primitive. The original `cv_text`, `cv_rect`, `cv_line`, `cv_begin` and `cv_blit` algorithms remain intact.

The total loaded `.text` extent falls **580,796 → 579,308 B**, exactly the 1,488 B image delta; `.ram_text` stays 2,956 B, `.c1_text` 160 B, `.data` 384 B, `.bss` 97,116 B, `.lowram` 28,928 B and `.pool` 335,424 B. Boot literal payload/report is identical (5,500/2,500 B destinations); no new literal-restoration allocation contributes to this gain.

Selected emitted symbol sizes illustrate why source-level estimates are unreliable:

| Symbol | Baseline B | Full candidate B |
| --- | ---: | ---: |
| `layer_screen_draw` | 5,944 | 6,016 |
| `draw_columns` | 2,178 | 2,178 |
| `draw_parameter_column` | 456 | 456 |
| `page_desc` | 434 | 434 |
| `graph_guide` | 476 | 422 |
| `draw_menu` | 1,762 | 1,630 |
| `draw_page_help` | 746 | 604 |
| `drum_screen_draw` | 2,084 | 2,024 |
| `drum_screen_input` | 2,132 | 2,132 |
| `layer_knobs` | 948 | 948 |

For example, band substitution makes the large layer function **grow 72 B**, while the complete image shrinks. Symbol deltas are diagnostic and must not be added to image savings. Full symbol/section inventories are in `ui/target-inventory.json`.

## Call chains, control ownership and larger structural findings

The real `ui_draw` dispatcher gives menus, held layers, recording, visualizers, action views, native drums, sequence views and TRACKS their owned paths before ordinary columns/graphs. TRACKS still calls `studio_tracks_draw`, whose pattern body shares `graph_home`; ordinary columns remain live for parameter, browser, STEP and slot pages. The obsolete TRACKS column and graph-forwarding paths from the earlier audit are already absent. This audit removes **no page or event path**. Direct-call scarcity was not used as dead-code proof.

Field resolution is already shared end to end: ordinary encoder edits call `page_desc` in `ui_input.c`; ordinary cards call it in `draw_columns`; help calls it in `draw_page_help`; input ownership probes also consult it. HOME uses `home_param`. `page_desc` already centralizes runtime-family values, VOICING, dynamic delay DRIVE/COLOR versus BITS/HOLD, drum FX global level/sends versus track PAN, hidden reverb TIME, recording snap and engine descriptors. That resolver emits only **434 B**. Replacing its switches alone cannot justify a multi-KiB savings claim.

Formatting still has intentional context: cards override CPU percentage, six-letter reverb models, recording DIV LIMIT, drum LEVEL, delay TONE, LOFI field labels and SCALE LATCH. Help has its own special-label policy. Input additionally owns acceleration/name stepping, MIDI learn, parameter locks, pending actions, undo and live reload rules. A descriptor/value pointer must not silently replace those action identities. This candidate retains every one of these paths, including help's current pixels.

`draw_column` and `te_dials` already share the NEON `ui_card` artwork. The former also populates `ui.big_*` and focus caches consumed by `graph_big`, `graph_signature` and zoom popups; these caches remain live. The latter hashes and blits a four-card strip. Combining their caches would change dirty granularity and potentially redraw/focus behavior. Likewise, the two browser bodies have distinct audition transactions and refresh policies; their superficially matching footer is not evidence of a multi-KiB duplicated browser engine.

The 5,944 B layer constructor is the largest remaining UI concentration. Its FX piano tiles, ERASE/ROLL note/drum presence, STEP velocity/ratchet/lock/nudge, SCALE chord qualities/guide roots and SONG section/chain/save ownership are different live work. Four modulation layers already share `live_mod_t`, input and `live_mod_draw`. They bypass tile rendering, but `layer_sub_shown` remains observable and tile storage must be initialized for all actual tile consumers. Avoiding initialization on modulation views is an **unmeasured CPU cleanup**, not an established flash win.

The bounded guide function is only 476 B, and existing parameter/NEON artwork sharing is already present. A blanket scene-command engine would add dispatch/table bytes and harder ownership/clipping review. It was not introduced to manufacture a larger apparent code deletion.

**Deferred, unmeasured larger candidate:** convert one complete layer/parameter family to a shared four-field recipe consumed by rendering and encoder handling, with explicit resolution, format policy and edit/action identity. Start with ordinary global delay/LOFI + drum FX, then held modulation; leave STEP/music transactions separate. A fixed descriptor table could remove repeated field/range/ratio assembly, but its ROM pointers, callbacks, stack model and remaining special cases must all be linked. Require **at least 2 KiB actual net reduction** before expanding this architecture, as the earlier architectural audit proposed. No new numerical savings estimate is established here. Do not finance it with the 804 B main gap, low remainder or pool reserve. No executable overlay or compressed-code proposal is qualified by this audit.

## Runtime, stack and ownership costs

This trades call overhead for flash. Each converted non-tail text/rectangle/line site adds **one wrapper activation** before the same primitive; palette wrappers call the primitive directly, not via `cv_text_s`. Target disassembly shows these wrappers saving `rets,r4` plus a four-byte outgoing slot: **12 B local activation**. Original five-argument callers already require an outgoing argument, so 12 B is not a proven whole-path high-water delta; changed caller register allocation must also be included. The band wrappers tail-branch with no additional stack activation. No wrappers nest each other.

No hardware cycles, instruction-cache gain, frame-time gain or absolute stack high-water is claimed. The full UI can issue many such calls during two clipped passes, so this is not a free runtime optimization. Core-0 UI owns the same canvas/palette state throughout; wrappers introduce no mutable persistent state, IRQ ownership, startup phase, cross-core scratch or audio callback. Palette/color reads move a few instructions into a synchronous helper, with no intervening setter/callback in these audited argument expressions. Upstream merges need to retain wrapper APIs and convert new UI call sites deliberately; ordinary primitive calls remain supported.

Before integration: measure core-0 stack watermark and worst-case UI response while maximum audio, recording, USB/MIDI, visualizers and rapid page/layer activity run. Verify NEON and custom themes, popup/full redraw and held-key/encoder feel on device. Keep the 350 ms display delay and immediate action ownership intact. Static RAM equality does not qualify dynamic stack or deadlines.

## Validation and reproduction

Completed evidence:

- Exact 584,296 B baseline reproduction and all independent/combined links above.
- Existing target entry/placement/ROM-reference checks: 925 call-free RAM-text instructions; worker 160 B; persistent scratch containment retained.
- Existing target DSP budget checks pass (`ui/target-budget.log`). These are static estimates, not device cycles.
- Baseline `ui_pages_test` and `ui_refresh_test`; both font+palette and final candidates pass **ui_pages, ui_refresh, ui_shared_paths, delay_ui, drum_fx_ui, upstream_controls_ui, piano_roll**. Assertions enabled with host wrapper, `-UNDEBUG -O2 -fwrapv -fno-strict-aliasing`.
- Actual UI navigation/held layers/recording/drums/song plus **20,000-frame fuzz**, with audio mixing between frames and bounded LCD blits; incremental/full redraw, palettes, help, drum union and modulation state pass. Piano-roll suite covers 8/16/32/64 viewports, clipped sustains, held tools, short patterns and unchanged music.
- **117/117 baseline framebuffer captures byte-identical** for both tested candidates. Candidates additionally produce 20 focused captures; those have no frozen-baseline comparison. `ui/validation/comparison.json` records the comparison, not a claim of 137 baseline comparisons.
- `ui/evidence.py` parses balanced calls and expands all wrapper substitutions: **all 14 UI C files reconstruct the original source modulo whitespace** (`call-expansion-proof.json`). Original rasterizers unchanged. The ordered patches also apply cleanly after the documented line-ending normalization and reconstruct the final C source.
- Private original-rasterizer oracle compares every pixel and returned x for **8,400 clipped text scenes**, each through palette and generic-small-font wrappers, across six palettes, random canvas heights/positions/y offsets. Exact **349 ms hidden / 350 ms visible** and immediate held encoder ownership pass. Same oracle passes UBSan with no recovery (`oracle*.log`). Rectangle/line/band paths are covered by source reconstruction, framebuffer parity and UI fuzz.
- Existing boot-literal target oracle passes: original/transformed objects, relocated final addresses, three real cold restores, guard bytes and UBSan (`literals-target.log`).
- Frozen snapshot manifest remains unchanged. No audio goldens were changed or relaxed. A full 200-golden audio rerun, WASM build and hardware timing/boot tests were not performed; no audio code or factory bytes are in the patch.

Reproduce target builds from PowerShell, never from the shared production source:

```powershell
wsl -d Ubuntu --cd /mnt/c/code/sloop-fm1/build/flash-audit-91/ui/primitives-source --exec env JIELI_TOOLCHAIN=/home/shaun/.cache/sloop-host-tools/jieli-linux-toolchains-20250324.1 FELUCCA_AUDIO_DIAG=0 python3 build/build-target.py
wsl -d Ubuntu --cd /mnt/c/code/sloop-fm1/build/flash-audit-91/ui/primitives-source --exec sh build/ui-tests.sh
wsl -d Ubuntu --cd /mnt/c/code/sloop-fm1/build/flash-audit-91/ui --exec env JIELI_TOOLCHAIN=/home/shaun/.cache/sloop-host-tools/jieli-linux-toolchains-20250324.1 FELUCCA_AUDIO_DIAG=0 python3 independent.py
python build/flash-audit-91/ui/evidence.py
python build/flash-audit-91/ui/compare.py
```

`baseline-source/build/ui-tests.sh` recreates baseline captures. The final private clipping/hold test is `primitives-source/tests/private_ui_oracle.c`; compile with the same host flags, optionally `-fsanitize=undefined -fno-sanitize-recover=all`. `primitives-source/tests/pool_literals_test.py --target` uses its private `build/host/optimization-cc` and `build/voice-experiment/` directory. The delivered snapshots contain both prerequisites. Initial prototype compile errors and missing private oracle prerequisites were corrected before the successful builds/tests; unsuccessful logs are not measurements.

The existing `note_chain_actions.c` missing-braces warning remains. No budget or assertion was weakened. Integration must rebuild the actual combined dirty parent source and rerun UI/target checks; this .91 saving is not additive to other chats' standalone results.
