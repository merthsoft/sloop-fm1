# Cold native UI headroom audit

Frozen working source: MerthSloop 2.7.1.49 on `merthsloop`, including the uncommitted parameter-icon removal and two-icon header atlas. This is **not a HEAD comparison**. Production firmware, shared generated assets, shared build outputs, refs and packages were not changed. All experiment artifacts are under `build/headroom-ui-51/`.

## Recommendation and measured ranking

**Integrate only `build/headroom-ui-51/dead-track-columns.patch`: 496 application bytes saved, no RAM increase, 113 identical framebuffer captures and passing native UI fuzz/redraw tests.** The result is 580,800/584,336 B, leaving 3,536 B free. Rebuild against the eventual combined source; do not add this result to another specialist's measured saving.

| Rank | Candidate | Same-version image | Delta | Disposition |
|---|---|---:|---:|---|
| Baseline | Frozen current working source | 581,296 B | — | Reproduces supplied baseline exactly |
| 1 | Remove bypassed old SC_TRK column renderer | **580,800 B** | **−496 B** | Recommended, bounded deletion; private patch supplied |
| 2 | Share the two browser footer drawing blocks | 581,280 B | −16 B | Measured, reject for headroom; private patch retained as evidence |
| 3 | Remove redundant GR_TRK forwarding inside draw_graph | Not built | Unknown | Source-proven bypass under current dispatcher, but tiny follow-up; no saving claimed |
| 4 | Shared display-field label resolution for help/cards | Not built | Unknown | Maintenance opportunity, not evidence of multi-KiB savings |
| 5 | Avoid layer tile initialization on modulation-only views | Not built | Unknown | Dead per-frame work on that route, primarily CPU cleanup; no flash claim |

All three measured rows have main static RAM **89,128/98,304**, low RAM **28,280/29,496**, pool **332,876/344,064** and the existing 8,192-byte pool reserve. Low RAM remains only 1,216 B free. No new decoder, heap object, view-model array, persistent field or audio-path helper is introduced. The footer candidate and deletion were measured independently, each against the same baseline; they were not combined.

## Freeze and evidence

`manifest.json` records SHA-256 for every copied tracked working file. Sources were copied from the filesystem using `git ls-files`, not exported from HEAD. `source/build/gen/` is a private copy of the current generated assets; normal target preset generation runs only there. The target compiler is `/home/shaun/.cache/sloop-host-tools/jieli-linux-toolchains-20250324.1` in WSL Ubuntu; SDK is the read-only shared `build/deps/ac79`. Source, version, compile flags and asset inputs are held constant between trials.

Evidence directories:

- `baseline/`: target ELF, binary, disassembly and linker-check notes, 581,296 B.
- `footer/`: independent footer trial, 581,280 B.
- `dead-track/`: old-column deletion trial, 580,800 B.
- `source/`: reproducible private source, `audit_build.py`, `audit_test.sh` and final candidate outputs in its own `build/`.
- `baseline-symbols.json`, `footer-symbols.json`, `dead-track-symbols.json`: parsed target ELF symbol sizes/addresses/types.
- Corresponding `*-sections.json`: complete ELF section accounting.
- `callers.json`: actual target call instructions attributed to containing function address ranges, including internal jump-table labels.
- `validation/{baseline,candidate}/`: host binaries, logs and PPM captures; `validation/comparison.json` records 113 versus 113 captures with zero differences.
- `target-budget.log`: target DSP/ISR budget results for the deletion.
- `browser-footer.patch`, `dead-track-columns.patch`: reviewable independent experiments; `ui_*.original.c` preserve baseline versions, `ui_draw.candidate.c` preserves the tested candidate.

These snapshots contain private generated assets and outputs. Do not run `tools/build.py` from the shared repository for reproduction of this audit.

## Target inventory and actual callers

Sizes below are emitted target function sizes, not whole-feature costs or removable byte estimates. Inlining and alignment prevent summing them as savings.

| Target symbol | Bytes | Actual role |
|---|---:|---|
| `layer_screen_draw` | 5,872 | Held layers, distinct musical tile/card construction |
| `draw_columns` | 2,534 | Ordinary cards, STEP, browser, project/user slots; includes bypassed TRACKS branch |
| `drum_screen_input` | 2,164 | Grid/kit/sound/groove/rhythm controls and audition/hold ownership |
| `draw_menu` | 1,850 | Settings/about, two canvas bands |
| `sequence_screen_input` | 1,374 | Starter/custom/progression browsing, preview/apply/merge and live performance |
| `sequence_screen_draw` | 1,328 | Starter/custom/live progression visual content |
| `layer_knobs` | 914 | Held control edits |
| `groove_screen_draw` | 900 | Drum masks, lane/cursor/rhythm and audition footer |
| `draw_graph` | 710 | Graph dispatch and popup/top-region policy |
| `draw_page_help` | 612 | Labels plus two clipped 120-row passes |
| `graph_signature` | 596 | Dirty detection for graph and focus/large values |
| `draw_column` | 484 | Fit/cache/focus/large-value production and NEON card draw |
| `tiles_draw` | 478 | White-key tiles, marks, tags, fill conditions and hints |
| `draw_parameter_column` | 466 | Ordinary descriptor formatting and special display overrides |
| `draw_foot` | 422 | Page identity/count/scope and gesture hints |
| `graph_big` | 370 | Active two-by-two or centered full parameter values |
| `punch_tiles_draw` | 330 | 27-key keyboard geometry and effect state |
| `te_dials` | 196 | Shared four-card rendering and dirty signature |
| `str_hash` / `studio_hash` | 18 / 14 | Different algorithms, both live |

Production target has exactly one call site each for `draw_columns` and `draw_graph`, both in `ui_draw`. `studio_tracks_draw` has two: the early `ui_draw` dispatch and an older `draw_graph` forwarding guard. `graph_home` is called by both active HOME and active TRACKS views. `te_dials` already serves layer, recording, TRACKS, groove, drum and sequence views (seven emitted sites, including two layer sites). `draw_page_help` is called by `draw_menu`; `graph_big` by `draw_graph`. This is not a collection of wholly separate pre-NEON renderers.

## 1. Proven obsolete TRACKS columns

Source anchors in the frozen/original tree:

- `firmware/src/ui_draw.c:1045`: `ui_draw` tests `!ui.home && cur_page()->scope == SC_TRK`, calls `studio_tracks_draw`, runs timers, clears force and returns.
- `ui_draw.c:1069`: the only production `draw_columns` call is after that return. Earlier menu, held-layer, recording, visualizer, action, drum and sequence dispatches do not call it.
- `ui_draw.c:815`: old SC_TRK branch constructs TRACK / LEVEL / LEN / PAN columns. Its first card changes track index, unlike today's SWING control. This branch cannot be reached through the current sole production caller.
- `firmware/src/ui_studio.c:114`: active TRACKS cards already come from `studio_tracks_draw`, showing swing/swing8, level, steps, pan through `te_dials`, plus the shared HOME pattern-lane body.
- `firmware/src/ui_input.c:307`: `tracks_edit` independently implements the real four controls, including unmute-on-level, global drum level and MIDI learn. The candidate does not touch it.

The deletion removes just that conditional block and its now-unused `unit` local. It does not remove `SC_TRK`, its page, input, help, visualizers, or the active renderer. HOME returns from `draw_columns` before the deleted location, so a retained TRACKS page pointer while HOME is active is harmless. Other regular pages retain all branches and formatting.

Target `draw_columns` falls **2,534 → 2,044 B** (490 B function reduction). `.text` falls **577,756 → 577,260 B** (496 B), accounting for the final image reduction; do not add those numbers. `.ram_text` remains 2,956 B, `.c1_text` 60 B, `.data` 524 B, `.bss` 88,600 B, `.lowram` 28,280 B and `.pool` 332,876 B. No other named function changes size; compiler anonymous merged-global numbering changes, so those labels must not be compared as stable identities.

Direct host calls to internal `draw_columns` with an SC_TRK page would lose that obsolete fallback. No production caller does this. The actual UI tests exercise TRACKS through `ui_draw`; preserving this unsupported internal rendering route would retain the dead target block. Future dispatcher changes must route TRACKS explicitly to its real renderer.

The `draw_graph` GR_TRK forwarding guard (`ui_draw.c:627`) is similarly bypassed in the real call graph: the sole GR_TRK page is SC_TRK (`params.c:467`). It remains in the recommended patch to keep the measured deletion narrow. It could be removed in a separately measured follow-up, but no byte estimate is justified here.

## 2. Browser footer prototype: only 16 B

`ui_studio.c:199` and `ui_sequence_starters.c:67` draw the same 240×38 footer at y=202 with a white first line and gray second line. The private helper accepts title and hint strings and preserves begin/text/blit order, dimensions, colors and caller redraw decisions. Both callers keep title construction and all state. The candidate saves only **16 B net target**, including helper overhead and alignment. No equivalent multi-KiB browser saving follows from source resemblance. For this headroom pass, reject integration as too little benefit for another boundary.

Do not share the browser transaction state machines merely to increase the apparent refactor:

- Drum preview snapshots seven masks, length/division and shaped phase; individual lane audition remains distinct. Musical preview holds starter/root/scale/mode/octave/voice-leading/rhythm/shape and must check held/latch ownership.
- Drum selection/page edits stop audition; musical page browsing deliberately preserves a running audition.
- Drum confirmation captures `groove_hold_sel`; global UI input cancels its timer on leaving the browser/layer ownership. Musical apply supports merge and returns CHORD FULL, with progression/live pages taking separate transport paths.
- The sequence body redraws each frame for live progression/slot output. Groove body uses a dedicated state signature. Combining those policies would either miss live changes or add unnecessary redraws.

Neither the footer trial nor a generic browser framework was promoted. Footer target placement passes; the footer-only trial was not subjected to the full host equivalence suite because its measured benefit did not justify integration. Do not describe it as fully validated.

## Live caches and intentional differences

**Parameter cards versus studio cards.** `draw_column` and `te_dials` both call the existing `gfx.c:314` `ui_card`; they do not contain two copies of the NEON card artwork. Ordinary columns fit engine names specially, join units, preserve per-column string keys, and populate focus plus `ui.big_*` before deciding whether to redraw. `te_dials` hashes all four values/labels/ratios/hot states and blits a whole strip. Merging them blindly changes truncation, unit spacing, dirty granularity or focus behavior. Existing shared `draw_parameter_column` is already integrated; no saving for that is re-proposed.

**Large/focus cache is live.** `draw_column` writes `ui.big_l/v/u/c`; `graph_big` reads them for GR_NONE/GR_INS pages, while `graph_signature` hashes them for redraw. `ui.focus_*` is consumed by the settings.zoom popup and its signature. NEON cards did not make these buffers obsolete. Removing the writes would break full parameter values or popup updates. The old parameter-icon dependency was removed before this freeze and contributes none of this audit's savings.

**Help metadata.** Help uses `home_param`/`page_desc` and therefore already shares parameter ownership with input. Cards still add drum LEVEL, delay TONE, LOFI type labels, SCALE LATCH and recording DIV LIMIT overrides. Help only has its own subset of display overrides (`ui_menu.c:102`); it can show descriptor labels rather than the displayed dynamic labels. A shared label resolver would reduce drift, but adopting it changes existing help pixels and must be an explicit behavior improvement with new expected captures. It cannot be claimed pixel-equivalent or a measured headroom win. No large descriptor struct/table was added speculatively.

**Menu/help bands.** Both render a screen in two passes because the canvas is smaller than the display. Repeating text calls across clipped bands is not duplicated linked code and is not a removable second settings implementation. The current menu also preserves settings-specific value colors, palette swatches, restart hints and CPU1 availability. Replacing it with ordinary cards would change behavior/layout.

**Layer tiles.** `tiles_draw` and `punch_tiles_draw` render different geometries: the former 16 white-key tiles with hint/marks/tag/fill condition; the latter 27-key piano layout with black-key outlines. Their hashes cover different semantic fields. Four modulation layers already share `live_mod_t`, knob handling and `live_mod_draw`; they are not four copied render engines.

**Dead per-frame initialization.** `layer_screen_draw` clears and initializes `tl` before its layer switch (`ui_layers.c:839` onward), even when `layer>=LY_VIB` ultimately calls `live_mod_draw` and returns before any tile draw (`:1231`). Conditional initialization could avoid this work for modulation-only views. It is a CPU cleanup, likely adding a branch rather than deleting much flash; no target saving or hardware timing is claimed. Keep persistent tile initialization for all tile consumers and preserve `layer_sub_shown` (host-test observable) if pursuing it.

**Hash helpers.** `studio_hash` is multiply-31 plus byte; `str_hash` is XOR-byte times 16777619. Both are emitted and have live callers. Replacing one with the other changes dirty signatures/collision behavior; the combined emitted bodies are only 32 B before caller effects. No recommendation to unify or remove them.

**Recording primitives.** `te_disc` remains used by the recording indicator and free-take display; `te_digit` remains used by the free-take seconds display. Their historical TE names/comments do not prove dead pre-NEON rendering. `rec_rows` still draws the recording track overview; it is not the ordinary TRACKS body.

## Validation and reproducibility

From PowerShell in the repository, run the target only with the private working directory:

```powershell
wsl -d Ubuntu --cd /mnt/c/code/sloop-fm1/build/headroom-ui-51/source --exec env JIELI_TOOLCHAIN=/home/shaun/.cache/sloop-host-tools/jieli-linux-toolchains-20250324.1 AC79_SDK=/mnt/c/code/sloop-fm1/build/deps/ac79 python3 audit_build.py
wsl -d Ubuntu --cd /mnt/c/code/sloop-fm1/build/headroom-ui-51/source --exec sh audit_test.sh
python tests/target_budget.py build/headroom-ui-51/source/build/felucca.dis tests/target_budget.txt
```

`audit_build.py` invokes `build_app` and `check` without shared generation, loader build or package publication. To reproduce baseline, copy `ui_draw.original.c` into `source/firmware/src/ui_draw.c`; for the recommended candidate copy `ui_draw.candidate.c`. The footer patch is an independent trial: restore all originals first, then apply that patch only inside the private snapshot. Preserve result files before each rebuild. `audit_test.sh` makes baseline/candidate native binaries sequentially using installed Zig 0.13 and leaves the candidate source selected. Its output directories are private. Shell script uses LF line endings.

Completed checks:

- Exact supplied baseline reproduction at 581,296 B.
- Target entry/ROM-reference/region checks; 925 RAM-text instructions, zero calls; CPU1 loop 60 B; zeroed persistent scratch 8,800 B remains within BSS.
- All target DSP budgets pass after the deletion; ISR static estimate remains 256, with three loop call sites. These are static estimates, not hardware-cycle measurements.
- Baseline **and** candidate actual-UI 20,000-frame fuzz pass, with audio mixing between UI frames, held layers, layer lock, recording, drums and gestures exercised.
- Baseline and candidate incremental/full redraw, palettes, modulation state, drum union, song/record transitions and help checks pass.
- **113 framebuffer captures per build, every byte identical**, including UI refresh captures as well as page/layer/recording/visualizer captures.

The pre-existing `note_chain_actions.c:24` missing-braces compiler warning reproduces in both target builds. An unused local warning introduced by the initial deletion trial was removed in the delivered patch and final confirmation build. No tests or goldens were relaxed. No flash, package, hardware startup, audio deadline timing, exhaustive musical state proof or full audio golden rerun is claimed; this candidate removes a cold unreachable renderer branch and does not change audio code.

Integrate the 496-byte deletion after parent review and combined-source target/UI checks. Keep the browser/footer framework, hash unification and wholesale cache deletion out of this headroom change.
