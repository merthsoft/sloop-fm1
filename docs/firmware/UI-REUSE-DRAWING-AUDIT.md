# UI reuse audit: drawing and layout

The recommended patch is now integrated with the controls and held-page candidates. [Actual combined measurements and validation](UI-REUSE-INTEGRATION.md) supersede standalone savings for the production build; the audit evidence below retains its frozen .98 baseline.

2026-10-29. **Recommended private candidate: 208 B net flash saved, no static RAM growth.** The frozen dirty .98 application reproduces at **584,288 / 584,336 B**. The combined drawing candidate links at **584,080 B**, leaving **256 B**. This is a small measured cleanup of an already substantially shared UI, not evidence for an unimplemented multi-KiB generic renderer.

## Scope and isolation

Copied the current working `firmware/`, `tools/`, `tests/` and generated headers into `build/ui-reuse-98/drawing/source/`; this is not HEAD or the older .91 audit source. The frozen manifest covers **382 files**, all unchanged after the audit. Production firmware, shared generated headers/build outputs, installer, version and Git state were not changed. This report is the only write outside that private directory.

Read `FLASH-91-UI-AUDIT.md`, `FLASH-91-INTEGRATION.md` and `DUPLICATE-CODE-AUDIT.md`. Their integrated small-font/palette wrappers, rectangle/line wrappers, full-width canvas bands, shared NEON cards and removed obsolete TRACKS rendering are already in this baseline and are **not counted again**. Drawing audit boundaries were `gfx.c`, `ui_draw.c`, `ui_studio.c` and `ui_vis.c`; other agents own page/control descriptors and held screens/menus.

Pinned WSL Ubuntu JieLi toolchain: `/home/shaun/.cache/sloop-host-tools/jieli-linux-toolchains-20250324.1`. `run.py` invokes only the frozen `build_app()` and target checks with `FELUCCA_AUDIO_DIAG=0`, normal production flags and the existing literal/preset transforms. No loader/package, live flash, SDK update, alternate compiler flags, sample packing, guard reduction or added restored-literal RAM.

## Linked measurements

Every row retains main static **97,612 / 98,304 B**, low RAM **28,928 / 29,392 B**, pool **335,424 / 344,064 B**, the 8,192 B pool reserve and unchanged `.noinit`. Standalone savings are alternatives/diagnostics; do not add them to other agents' results or a different parent build.

| Candidate | Image B | Saved vs .98 B | Disposition |
| --- | ---: | ---: | --- |
| Frozen baseline | 584,288 | — | Exact parent reproduction |
| Shared S/L width calls, ordinary generic implementation | 584,240 | 48 | Superseded by fixed-width implementation |
| Shared S/L widths with fixed-font fast path | 584,224 | **64** | Recommended slice |
| Shared generic tail-trim loop | 584,272 | **16** | Recommended in combination |
| Residual large-font/palette text setup | 584,256 | **32** | Recommended in combination |
| Fixed REC digit rectangle recipe | 584,192 | **96** | Recommended slice |
| All four recommended slices | **584,080** | **208** | Recommended combined patch |
| General `text_w` fixed-font fast path | 584,304 | **-16** | Reject for flash; runtime idea only |
| Shared white outline for roll selection / drum pad | 584,288 | **0** | Reject: no linked saving |

The standalone four selected deltas happen to sum to the combination's 208 B here; the combination was actually linked. The general width fast path and ordinary width wrappers are alternatives, not additional gains.

Baseline application SHA-256: `2e8ab6189ef96ba24565961c2d8e2b5d3659deabd1e32f1a0d483df266ffd238`.

The final repeat is byte-identical. Combined application SHA-256: `65255db7331194b37271ee81bc8262188ecf04f6866b66669fce52f4bf461d27`.

Private integration patch: **`build/ui-reuse-98/drawing/combined.patch`**; full candidate: `combined/`. Apply only to a review copy matching this dirty .98 source. Source line endings are normalized in the patch; do not apply it atop another variant. Retain ignored evidence before cleaning `build/`.

## What is actually reused

### Width policy: `gfx.c:text_w_s/text_w_l`

Seventeen literal-font width queries in the audited files share two one-argument helpers. Each helper obtains its font's actual `fixed_adv`; no numeric glyph width is duplicated in callers. For these generated monospaced fonts, the compiler folds the helpers to `str_len(s) << 3` and `str_len(s) << 4`. If a future font supplies an advance table, the source retains the original `text_w` fallback. Unknown glyphs, lower case folding and Latin-1 substitution have identical advances for the current fixed-width fonts, so this shortcut preserves widths for every input byte. Dynamic-font layout retains the generic implementation.

The actual helpers are **10 B each**. The generic original remains **34 B** for dynamic users. This additionally avoids a `glyph` and `font_advance` call per character at these fixed-font queries. That is an instruction-path observation, not a target cycle/deadline measurement.

### Truncation: `gfx.c:text_trim`, `ui_draw.c:fit`

The bounded value truncation in `ui_card` and the existing copy/fit function now share one 40 B helper. It preserves removal from the end, the supplied font/max-width, the empty-string guard and all buffer capacities. `fit` remains the eight-byte copy policy and tail-branches to trimming; callers do not acquire a new copy buffer. The card's short label retains its narrower fixed-font loop rather than routing every case through generic width work.

### Residual font setup: `gfx.c:cv_text_l`, card label

The few remaining large-font calls in the audited drawing/studio paths share one 24 B wrapper. The card label reuses the already existing palette-aware small-font wrapper. This does not recount the 149 small-font substitutions from .91. Rasterizer, return x position, current palette reads, clipping, order and canvas/blit behavior remain unchanged. No text style registry or descriptor interpreter is added.

### REC countdown: `ui_studio.c:te_rec_digit`

The former seven-segment painter exposed variable geometry/color, but its only two callers both request the same **32×56, thickness 7, y=4, white** countdown digit. A seven-entry rectangle recipe and one segment loop now express that actual composition once. The recipe retains all seven original rectangles, digit mask order, overlap and integer midpoint rounding, including the middle segment's y=25 relative offset. Both tens and units call this one painter.

The emitted function shrinks **160 → 66 B**, plus a 28 B constant recipe and narrower call sites. The full linked image saves **96 B**, not the raw function-size delta. The table is ordinary ROM, not startup-restored data. This preserves countdown appearance and supported numbers; it does not remove a user-visible graphics feature.

## Existing reuse and rejected generalizations

- `ui_card` already renders ordinary parameter cards and the studio four-card dial strip. The callers have different dirty caches and focus/large-value state; merging those state machines would change redraw/interaction ownership.
- HOME track lanes and studio track view already share `graph_home`; track naming/level helpers also serve layers. Visualizers share FFT/band snapshots, level conversion, color interpolation and canvas primitives. Different waveform, spectrum, spectrogram, trail, groove and journey semantics are live work rather than duplicated screens.
- `str_hash` is FNV while `studio_hash` is a multiply-by-31 signature. They look similar but changing one changes dirty-cache collision behavior. A parameterized loop would add mode setup/branches to many short strings; no clean measured win was established. Not proposed.
- `te_disc` and `vis_ellipse` have different boundary rules (`r*r+r` versus elliptical scanline inclusion). They are not pixel-identical duplicate circle implementations; substituting either without a visual change is invalid.
- A shared white frame helper replaces two four-edge outlines cleanly but yields **zero** net image reduction. The original rectangle compositions are retained in the recommended patch.
- A generic fixed-font fast path in every `text_w` call might reduce UI computation, but increases the linked image **16 B**. The narrower fixed-font helpers yield both reuse and smaller call setup without changing generic font handling.
- A general scene/command/table renderer could replace more source text, but would add dispatch, tables, parameter marshalling, clipping/dirty ownership and stack costs. There is no fresh measured multi-KiB saving to claim here. Keep family-specific behavior explicit unless an isolated complete conversion produces a real linked win.

## Validation and performance limits

The full combination passes:

- Assertions-enabled baseline and candidate **`ui_pages_test`**, including all pages/layers/REC/drums, button/encoder ownership and **20,000 random frames** each. All **79 captured framebuffers** compare byte-for-byte; no clipping assertion fires.
- Existing **`ui_gfx_sharing_test`** on both baseline and candidate: palettes, clipped text, return x positions, 349/350 ms held-page timing and immediate held-encoder ownership.
- Private UBSan **`drawing_reuse_oracle.c`**: **12,000 randomized cases** across all six palettes. Legacy formula references compare fixed S/L widths for mixed nonzero bytes including missing/Latin-1 glyphs, generic trimming with fixed and artificial proportional metrics, complete cards and every REC digit under varying positions, canvas heights and `cv_oy` clipping. Pixels and strings match exactly. Shift sanitizing is excluded consistently with the fixed-point host harness; other undefined behavior aborts.
- Full target placement/MMIO/core-1/RAM checks and all target budget caps. No audio code, sample bytes, routing, allocator, persistence layout or voice budget is changed.

Evidence is retained in `validation/`, `inventory.json`, per-variant `checks.json`, `commands.json`, `sections.txt`, `symbols.txt`, `felucca.dis` and `budgets.txt`. `host.py`, `oracle.py`, `run.py`, `evidence.py` and `check_frozen.py` reproduce the checks without writing shared outputs.

**Runtime/stack:** fixed-font widths replace repeated glyph calls with one short string scan. The digit loop adds at most seven mask/recipe iterations per countdown repaint, while eliminating repeated geometric setup; no display-rate or hardware cycle claim is made. Target prologues retain the card's 68 B local frame and the digit's 20 B frame; `draw_column` falls 120 → 112 B. The shared trim adds a 20 B frame while called from the card; `fit` releases its own 8 B frame before tail-branching. The large-font wrapper adds a 12 B frame around rasterization. These bounded cold-UI costs are real; whole-device stack high-water and hardware redraw latency were not measured. No audio-sample-loop call is introduced.

Before adopting, review the patch, rebuild the actual combined dirty parent source with the other audit candidates, rerun UI captures/navigation, and confirm the real final linked/RAM results. The **208 B** is not an additive promise against other changes.
