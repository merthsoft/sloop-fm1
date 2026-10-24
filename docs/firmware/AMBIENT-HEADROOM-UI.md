# Ambient/drone headroom: current UI audit

2026-10-24, frozen working release **2.8.0.58**. No production source, generator, shared generated asset, installer, package, version or Git state was changed by this audit. Experiments and reviewable patches are under ignored `build/headroom-ambient/ui/`. The snapshot uses the dirty filesystem inputs, not HEAD; a subsequent parent commit does not change this comparison.

## Decision

**No substantial new UI flash saving was found.** The safe dead-route cleanup saves **16 linked application bytes**. A separate bounded tile layout saves **112 main RAM bytes**, with **zero flash saving**. Their actual combined link saves 16 flash / 112 main RAM bytes. Neither replenishes the 568-byte low-RAM margin. Do not budget the historical 4–10 KiB UI estimate for ambient/drone work: it remains unproved after the previously integrated cleanups and this matrix.

Use `dead-graph.patch` if a small cleanup is worthwhile. Treat `tile-bits.patch` as optional RAM housekeeping, not an ambient flash solution. `combined.patch` contains both; it is a private candidate, not an applied production change. No features, sounds, NEON artwork, persistent formats or upstream project compatibility were removed.

## Frozen build and ranked measurements

All rows are independent baseline trials except the explicitly combined row. Application capacity is 584,336 B; main RAM 98,304 B; low RAM 29,496 B; pool 344,064 B with the existing 8,192-byte reserve. Positive deltas consume space.

| Priority / disposition | Trial | App B | App delta B | Main RAM B | Main delta B | Low RAM B | Pool B |
|---|---|---:|---:|---:|---:|---:|---:|
| Baseline | Frozen current working files | 583,388 | 0 | 86,176 | 0 | 28,928 | 332,876 |
| 1, bounded cleanup | Bypassed TRACKS graph forwarding | 583,372 | **−16** | 86,176 | 0 | 28,928 | 332,876 |
| 2, optional RAM only | Tile flags use their proven bit widths | 583,388 | **0** | 86,064 | **−112** | 28,928 | 332,876 |
| Actual combination | Both above | **583,372** | **−16** | **86,064** | **−112** | **28,928** | **332,876** |
| Reject for headroom | Common draw-exit/timer path | 583,388 | 0 | 86,176 | 0 | 28,928 | 332,876 |
| Reject for headroom | Explicit `cv_rect` boundary | 583,388 | 0 | 86,176 | 0 | 28,928 | 332,876 |
| Reject for headroom | Explicit `cv_text` / `text_w` boundaries | 583,388 | 0 | 86,176 | 0 | 28,928 | 332,876 |
| Reject for headroom | Explicit `cv_line` boundary | 583,388 | 0 | 86,176 | 0 | 28,928 | 332,876 |
| Reject | Explicit canvas begin/blit boundaries | 583,404 | +16 | 86,176 | 0 | 28,928 | 332,876 |
| Defer, no flash benefit | Skip modulation-only tile initialization | 583,388 | 0 | 86,176 | 0 | 28,928 | 332,876 |

Combined headroom is **964 B application**, **12,240 B main RAM**, **568 B low RAM** and **11,188 B gross pool** (2,996 B beyond the required reserve). These are final measured occupancies, not summed function-size estimates.

Baseline → combined sections: `.text` **580,072 → 580,056**, `.ram_text` **2,956 → 2,956**, `.c1_text` **60 → 60**, `.data` **300 → 300**, `.bss` **85,872 → 85,760**, `.lowram` **28,928 → 28,928**, `.pool` **332,876 → 332,876**. Main RAM includes linker alignment; do not infer it by adding only `.data` and `.bss`.

## Source proof and tradeoffs

### Dead TRACKS graph forwarding

`params.c` has one GR_TRK descriptor, the SC_TRK TRACKS page. The only production `draw_graph()` caller is the ordinary path at the end of `ui_draw()`. Before it, `ui_draw()` explicitly handles `!ui.home && scope == SC_TRK` with `studio_tracks_draw()`, timer advancement, force clearing and return. The matching guard inside `draw_graph()` can therefore never execute through production dispatch. HOME with a retained TRACKS descriptor is excluded by `!ui.home` in both guards.

The deletion keeps the active TRACKS renderer, knobs, visualizer, help, HOME lanes and graph redraw behavior. Target `draw_graph` shrinks **704 → 688 B**, exactly matching the 16-byte image reduction. The old direct internal call `draw_graph()` on a non-HOME TRACKS page ceases to be a fallback; it is not a production entry point. Future dispatcher changes must retain explicit TRACKS routing. Upstream conflict burden: one five-line deletion, low, but applicable only where the specialized early TRACKS dispatch exists.

### Tile flags: RAM recovery, not flash recovery

`tile_t` is transient renderer-owned metadata, never persisted, transmitted, or shared with audio. The single static 27-entry tile array is rebuilt before rendering. Every assignment proves the ranges: synth ratchet is `rat & 3`, drum ratchet accessor returns two bits, `step_locked()` returns 0/1, and fill is reduced modulo three. Allocate `marks:2`, `tag:1`, `cond:2` within `uint8_t` instead of three separate bytes. Labels, pointers, colors and renderer conditions remain unchanged. There are no addresses taken of these members, no raw structure hashing/serialization and no cross-compiler ABI contract.

On this target, tile size drops **24 → 20 B**, so `layer_screen_draw.tl` drops **648 → 540 B**. The complete main RAM region drops **112 B**, including alignment; do not call the 108-byte object delta a separate gain. `layer_screen_draw` shrinks **5,986 → 5,942 B**, while `tiles_draw` grows **478 → 522 B** due to field extraction. These cancel in the image. No unaligned pointer packing or audio-path decoding is introduced.

Upstream conflict burden: three renderer-only declaration edits, low. Retain the bounds if adding tile metadata later. This is optional because main RAM is not the limiting region and UI instruction scheduling changes have no hardware timing evidence. Do not describe it as an unchanged-cost or faster renderer.

### Rejected experiments

The explicit helper-boundary trials use the existing `UI_SMALL` convention only on named graphics helpers. The compiler already emits shared bodies for the principal primitives. Rectangle, line and text trials save nothing. Outlining canvas helpers grows the complete image by 16 B despite a smaller `draw_graph`; this illustrates why local function shrinkage is not linked savings.

The common-exit trial replaces duplicated early `ui_timers(); ui.force=0; return;` sequences with a shared label, keeping the regular footer ordering. Its target symbol sizes and image are unchanged: the compiler already shares this work effectively.

Skipping tile initialization on the four modulation views reduces `layer_screen_draw` by six emitted function bytes but saves **zero image bytes**. It avoids some source-level work on that route; no hardware speedup was measured. This was already identified, but not measured, in the previous UI audit. It is now measured and rejected as flash headroom.

## Remaining consolidation boundaries

Read the architectural, optimization, redundancy, previous UI, piano-roll, compiler/link and asset/persistence audit findings before selecting experiments. The obsolete pre-NEON TRACKS columns, shared parameter column, icon dependency removal, fixed font metrics, shared entry transaction and cold state initialization are already integrated. Their historical savings are excluded here. The prior browser-footer prototype saved only 16 B and is not promoted as a new result.

Ordinary cards and studio dials already share `ui_card`, rather than duplicate NEON artwork. Ordinary cards must still populate large/focus values and cache individual fields; studio strips hash/redraw four cards together. Both caches have live consumers. Tile and punch renderers use different keyboard geometry and cannot simply be merged without retaining their separate behavior. Recording discs/digits remain live. `studio_hash` and `str_hash` have different algorithms and collision behavior; their small bodies are not a worthwhile duplicate pool.

Display metadata still drifts between `draw_parameter_column()` and `draw_page_help()`: delay, LOFI, recording and drum labels have display-specific overrides. A shared resolver could improve consistency, but converting it is a behavior change for help, adds an interface/table boundary, and has no measured multi-KiB result. Do not sell it as immediate space. A future isolated conversion should resolve one complete page family for input/help/draw, include new tables in its link, preserve knob ownership and demand a material net result before broadening.

The editor's `ed_tdesc()` intentionally returns static protocol descriptors; its comment explicitly excludes the device-only engine descriptor hook. Reusing that path for dynamic panel labels would change the protocol/display boundary. No editor protocol, persistent project or DSP changes were made.

## Correctness evidence and limits

Target baseline and every matrix row pass existing entry, ROM-reference and region checks. RAM code retains **925 instructions with no calls**; CPU1 loop remains 60 B. Baseline and combined target DSP/ISR budget logs pass. These are disassembly checks, not measured hardware timing, stack high-water or perceived control responsiveness.

Native baseline and combined checks and framebuffer comparisons are recorded in `validation/`; the original refresh test has a stale LOFI expectation (`FLUTTR`) that fails on the unmodified baseline. That original failure is retained, not silently repaired in production. `refresh_current.py` makes a separate private expectation-only test covering the current seven LOFI types and their labels; its exact fixture patch is retained beside each test result. It changes no firmware.

Baseline and combined each pass `ui_pages` (including **20,000 frames** of mixed UI/audio fuzz), `ui_shared_paths`, `upstream_controls_ui`, `sequence_starters_ui`, `piano_actions_ui` and `delay_ui`. Each reproduces the same original `ui_refresh` failure. The separate current-label refresh suite passes both, including incremental/full redraw parity, palettes, drum union, modulation state, recording/song transitions and help. The original suites produce **125 captures per build**, all byte-identical; the separate updated refresh produces **36 per build**, all byte-identical. These capture sets overlap in screen coverage and are not claimed as 161 distinct screens. `comparison.json`, `comparison-current-refresh.json`, per-suite logs and `results.json` preserve the accounting. No hardware performance or sound claim follows from host tests.

## Reproduction and handoff

`manifest.json` hashes all 321 copied firmware/tool/test/generated input files. `original/` holds the three files touched by trials. `variants.py` always restores those originals first, then selects exactly one named variant; `combined` is explicit. `summary.json` records exact images, hashes and changed symbol sizes. Each named result directory contains ELF, binary, disassembly, symbols, sections and region checks. These binaries are audit evidence, not published firmware.

End-of-audit read-only verification finds all frozen production firmware and shared generated inputs unchanged. Two unrelated shared website tool/test files changed concurrently (`tools/web_provenance.py`, `tests/pages_deployment_test.py`); the private copies stayed frozen. `shared-input-verification.json` records this distinction. `dispatch-callers.json` records the actual target calls: `ui_draw → draw_graph`, `ui_draw → studio_tracks_draw` and the bypassed `draw_graph → studio_tracks_draw`.

Compiler: existing WSL Ubuntu JieLi `jieli-linux-toolchains-20250324.1`, Clang 4.0.1, `-target pi32v2 -Oz -ffunction-sections -fno-builtin -Wall -Wno-unused-function`. Tool executable hashes are in `toolchain-sha256.txt`. Default release feature configuration and product `FM-1_900`, unchanged version 2.8.0.58; two-pass boot presets and boot tables enabled. The copied `build.py` compiles only this private tree; `audit.py` calls `build_app()`/`check()`, not normal generation, loader/package builds or installer publication. Preset packing writes only private generated headers. Existing font/sample/icon assets are copied unchanged. No SDK or hardware operation is required for this image comparison.

From the repository, a target reproduction is:

```powershell
python build/headroom-ambient/ui/variants.py combined
wsl -d Ubuntu --exec python3 /mnt/c/code/sloop-fm1/build/headroom-ambient/ui/source/audit.py combined
```

For the baseline select `baseline` first; result directories are overwritten on a repeated measurement. Host tests use installed Zig 0.13, `cc -O2 -w` and private include paths/caches. Run `validate.py baseline` and `validate.py combined` only after selecting the intended source; the baseline runner explicitly restores original UI files in its own test copy. Then run `refresh_current.py` for each label. Preserve evidence before reusing a label.

**Parent handoff:** budget no more than **16 flash bytes** from this scope. Review `dead-graph.patch` independently. Only take `tile-bits.patch` if the 112-byte main RAM recovery has a use; it does not fund low-RAM metadata or appreciable new code. If combining with other specialists, remeasure the actual integrated source and rerun UI redraw/fuzz plus target checks. No version bump, commit, push or publication was performed here.
