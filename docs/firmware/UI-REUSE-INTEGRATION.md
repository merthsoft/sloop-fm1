# UI reuse integration

Three specialist audits reviewed the current dirty .98 source, rather than Git HEAD. Each reproduced the **584,288-byte** normal build and **97,612-byte** main static RAM usage before measuring candidates. Reports and private patches:

- [Drawing/layout](UI-REUSE-DRAWING-AUDIT.md): fixed-font measurement, shared text trimming/large-font calls and a compact REC digit recipe. Standalone saving 208 B.
- [Page controls](UI-REUSE-CONTROLS-AUDIT.md): shared parameter-turn arithmetic, one HOME macro identity lookup and common short/long subdivision label storage. Standalone saving 256 B.
- [Menus/held pages](UI-REUSE-LAYERS-AUDIT.md): common byte-setting stepping, held-FX value edits, one held-page dial dispatch and existing encoder draining reused by note actions. Standalone saving 128 B.

These are new savings beyond the earlier .92 drawing wrappers. The already shared parameter resolver, cards, tiles, live modulation screens and HOME track view were retained. Different visualizer geometries, hashing policies and screen state ownership were not treated as duplicate implementations. Several plausible helper extractions saved zero bytes or grew the image; they were excluded.

## Combined measurement

The actual combined source at the same .98 version links at **583,728 / 584,336 B**, leaving **608 B flash**. This is **560 B saved**, rather than the 592 B sum of independent candidates. Main static RAM remains **97,612 / 98,304 B (692 B free)**; low RAM **28,928 / 29,392 B** and pool **335,424 / 344,064 B** are unchanged. The 8,192 B pool reserve is retained. No feature, sample quality, voice budget or audio protection is removed.

## Implementation and limits

The packaged **2.8.0.99** build has the same **583,728-byte** image and unchanged RAM totals as the same-version combination. No additional feature removals are needed to fund this batch.

`param_turn` preserves one call to acceleration per edit, with the same encoder identity, enum handling, clamp ranges and state updates. HOME resolves the engine macro once for lock and MIDI-learn identity. Short subdivision enums use the six-entry prefix of the existing nine-entry sequence label table, retaining their original limits and IDs.

Fixed-font width wrappers scan once instead of repeatedly resolving glyph metrics; proportional fonts retain the generic fallback. Shared trimming preserves the original pixel limits. The REC countdown's two identical 32×56 white digits use a seven-rectangle recipe instead of a general painter with unused dimensions/color options. Menu stepping preserves behavior even for out-of-range stored bytes. Held-FX edits retain separate destination/range semantics, and held modulation screens still draw their plots after the common dial strip.

These changes trade repeated cold-UI code for small bounded calls. The parameter helper and shared trim each add a 20 B local call frame; the large-font wrapper adds 12 B. The REC digit frame remains 20 B, the card frame remains 68 B, and `draw_column` falls from 120 to 112 B. These are local prologue observations, not a whole-device stack high-water bound. No audio sample-loop call or allocation is introduced. Physical redraw responsiveness and stack margins remain hardware qualifications; no hardware speedup is claimed.

## Evidence

Combined-build outputs, original .98 screenshot comparisons and checks are retained under `build/ui-reuse-98/integration/`. The original specialist baseline/candidate trees and reports remain available. Three permanent exact-behavior tests cover parameter/encoder state, drawing and menu/held-FX state; the original formulas/frozen state digest remain their reference.

All eighteen combined-build programs pass: UI pages, refresh, shared paths, DUCK, song, note actions, upstream controls, graphics sharing, sequence navigation, track FX, delay, ambient DRIFT, the three new reuse oracles, HOME identity, menu captures and audio regression. The three reuse oracles run with UBSan and the existing fixed-point shift exception. All **122 broad + 138 focused menu/palette framebuffers match byte-for-byte**. Parameter/encoder comparison covers **218,120 cases**, drawing covers **12,000 cases**, the menu/held-FX digest matches **394d41c8**, the full UI suite completes **20,000 random frames**, and all **200 audio goldens remain unchanged**.

The integration driver initially stopped before running the last three programs because its screenshot-directory name collided with the `regress` executable. No assertion failed. After correcting the private driver, those three programs and framebuffer comparisons completed successfully; initial and resumed logs are retained separately, with the complete results in `validation.json`.

Final .99 target placement, audio-loop budgets/inlining caps and the master LOFI guard pass. The boot-literal oracle restores **1,411 complete objects** with final relocated addresses, three cold restores, guard bytes and UBSan. Native/WASM preview parity and playback/editing checks pass, as do all **16 Pages deployment tests**. The local installer is verified serving .99. Hardware UI responsiveness and stack high-water remain unmeasured.
