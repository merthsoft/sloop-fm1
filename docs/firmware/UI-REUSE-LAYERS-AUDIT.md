# Held-page, menu and action UI reuse audit

The recommended patch is now integrated with the controls and drawing candidates. [Actual combined measurements and validation](UI-REUSE-INTEGRATION.md) supersede standalone savings for the production build; the audit evidence below retains its frozen .98 baseline.

This audit freezes the current dirty **2.8.0.98** source, rather than committed
HEAD. The recommended private combination saves **128 linked flash bytes** with
**no static RAM or restored-literal growth**. It does not change production
firmware. The larger UI sharing work from the [.91 UI audit](FLASH-91-UI-AUDIT.md)
and [.92 integration](FLASH-91-INTEGRATION.md) is already present and is not
counted again.

## Scope and measurements

Reviewed `ui_layers.c`, `ui_menu.c`, `ui_song.c`, `ui_note_actions.c` and their
existing card, tile, formatting and encoder helpers. All sources, generated
headers and three pinned SDK packaging inputs are frozen beneath
`build/ui-reuse-98/layers/baseline-source/`. The original manifest covers 393
files; the frozen-file recheck has zero mismatches. All generated outputs,
compiler runs, package artifacts, experiments and tests remain private beneath
`build/ui-reuse-98/layers/`. Only this report is outside that directory.

Every target run uses the pinned JieLi toolchain, normal production `-Oz` flags,
`FELUCCA_AUDIO_DIAG=0`, the actual 584,336-byte slot and the existing reserves.
There are no feature removals, budget adjustments, compiler-flag experiments,
new startup allocations or DSP changes.

| Cumulative variant | Linked image B | Saved versus baseline B | Flash free B |
| --- | ---: | ---: | ---: |
| Frozen dirty .98 | 584,288 | — | 48 |
| Exact menu stepping + held-FX knob edits | 584,208 | 80 | 128 |
| Previous + one held-screen dial call | 584,176 | 112 | 160 |
| Previous + reuse action encoder drain | **584,160** | **128** | **176** |
| Alternative clipped-menu rendering, before drain sharing | 584,208 | 80 | 128 |

These are full links, not sums of estimated source reductions. All variants use
main **97,612 / 98,304 B**, low **28,928 / 29,392 B**, pool
**335,424 / 344,064 B**, with the existing 8,192-byte pool reserve retained.
The boot-literal report is identical: 10,672 original bytes, 9,782 pooled bytes,
1,410 symbols and unchanged 5,500/2,500-byte destinations; its payload digest is
`4aa553873647bed27d7831249f33f494a495f770dc1a0ebd654be7fa46617903`.

## Recommended narrow changes

The exact review patch is **`build/ui-reuse-98/layers/final.patch`**; complete
candidate sources are in **`final-source/`**. It touches only three UI files.
Normalize those files to LF in a private review copy before applying the LF
patch to a checkout with CRLF. Integration must remeasure the actual combined
tree; savings must not be added blindly to the other UI agents' measurements.

1. **Menu byte stepping:** RATE, VOICE BUDGET and the three lighting controls use
   the same clamp-at-ends/zero-direction-wrap behavior. One 42-byte leaf helper
   supplies it. The existing RESO/FADE clamp remains separate: although normal
   values behave similarly, its behavior for malformed out-of-range bytes is
   different. The preliminary broader merge is rejected for that reason.
2. **Held-FX knobs:** FILTER, DUST, DUCK and track FILTER choose their original
   value pointer, minimum and encoder, then use one acceleration/clamp/store
   expression. Each encoder keeps its own acceleration identity. FILTER bounds
   remain -64..63; DUST/DUCK remain 0..127. No pointer table or new storage is
   needed.
3. **Held-screen dials:** tile rendering remains specific to ordinary versus
   punch layouts; both paths reach one `te_dials` call. The modulation plot still
   draws after the dials. Title/header, footer, tile and plot ordering is intact.
4. **Action exits:** the two local seven-encoder drain loops call the existing
   `enc_drop`. They still consume every pending turn before another page handles
   input. No action, edit preflight, undo, recording or transport rule changes.

Actual emitted symbol sizes explain the scope: `mi_set` falls 590→520 B while
adding the 42-byte helper; `layer_knobs` falls 948→900 B;
`layer_screen_draw` falls 6,018→5,992 B; `actions_leave` falls 98→88 B and
`actions_input` 296→286 B. Alignment means these symbol deltas are not a substitute
for the linked-image figures. `linked-symbols.json` uses ELF symbol sizes, rather
than truncated disassembly spans.

The new menu helper is a leaf without a stack frame. Held-FX and layer-screen
entry frames remain unchanged (same saved registers and 28/132-byte locals).
Reusing `enc_drop` adds a cold call with its existing eight-byte activation on
action exits. This is not a demonstrated UI speedup or a physical stack
high-water qualification. No new per-sample/per-control DSP call is introduced.

## Already shared and intentionally separate

- The four live VIB/TREM/WAH/PAN popups already share `layer_knobs`, one parameter
  model in `layer_screen_draw`, `te_dials` and a common `live_mod_draw` path.
  Their separate branches mostly select labels, units, bounds and plots.
- All held pages already share title caching, full-width bands and the same
  16-tile renderer; punch tiles intentionally have shorter white tiles and
  overlapping black keys. Combining their geometry would need extra modes and
  arguments while preserving distinct hint, ratchet, condition and tag behavior.
  No multi-KiB saving is established by their visual similarity.
- Ordinary cards, SONG's four cards and held-page dials already use the shared
  `ui_card` artwork. SONG's entry/section/bars/count formatting is its own model,
  not another implementation of the artwork.
- Note and drum action sheets already share the renderer, input dispatch,
  readiness tests and apply lifecycle, with a small label table for each. Their
  musical move/copy/delete preflights are intentionally specialized.
- Menu values, section tabs and setting mutation already have central
  `mi_value`, `mi_sec`, `mi_set` and `mi_toggle` helpers. Replacing all setting
  branches with pointer/behavior descriptors is an unmeasured larger change,
  especially given inverted switches, deferred persistence, CPU1 behavior and
  palette/theme side effects.
- Section arrangement, held live-section launching and destructive confirmations
  share lower-level storage/arrangement APIs. They intentionally have different
  confirmation, transport and input-ownership behavior. Their lifecycle should
  not be unified merely because both display A..D.

## Rejected and unmeasured alternatives

**Measured, not recommended for flash:** skip menu tabs in the second canvas
band and format/draw each setting only in its intersecting band. All 138 focused
menu captures remain identical, but the target image grows **32 B** over the
corresponding dial-sharing candidate (584,176→584,208 B). It avoids known fully
clipped work but was not benchmarked on hardware. Keep it separate from the
flash patch.

**Rejected semantics:** using the new byte-step helper for RESO/FADE without
preserving malformed-value clamping. A smaller source expression is not enough
reason to alter this edge case. The final patch excludes it.

**Unmeasured:** defer initializing the 27-entry tile model when a live modulation
popup only needs dials/plot; investigate if that cold UI work matters after
measuring device timing. Replacing tile/song/menu/action rendering with a generic
view interpreter, or converting every menu switch to descriptors, has no measured
advantage here and would add substantial validation surface.

## Validation and reproduction

Private logs preserve the intermediate test runs as well as the final checks.
The focused UBSan state oracle compares **8,960** affected menu mutations
(all 256 byte values, seven directions, five controls) and **8,428** held-FX edits
(four knobs, 301 starting values and seven turns). Baseline and both tested
intermediates produce digest **`394d41c8`**. Tests retain nonrecovering UBSan with
only the project's inherited signed-shift exception. No assertions were disabled.

The baseline and final target placement/ROM/MMIO checks pass; target DSP budget
and all 25 hot-function inlining checks pass. Frozen source and candidate keep
identical literal payloads. Final integration still needs the actual combined
target, browser and hardware qualification; no device flashing or installer
publication was performed in this audit.

Both baseline and final candidate pass **ui_pages, ui_refresh, ui_shared_paths,
duck_ui, song_ui, piano_actions_ui and upstream_controls_ui**. This includes real
held-page/menu navigation, hold/tap and action-exit ownership, all twelve action
cases, incremental/full redraw parity, and the 20,000-frame bounded-blit fuzz
with audio between frames. **122/122 broad captures plus 138/138 focused menu
captures are byte-identical**, with no missing or extra captures. The menu oracle
visits every setting row and section across all built-in palettes, checks ABOUT,
and verifies unchanged incremental draws. `validation-summary.json` records the
final results, source recheck and linked image digest.

An early song harness run supplied a screenshot directory where that test expects
a filename and failed its `fopen` assertion. An early action invocation supplied
an unmatched case name and therefore did not exercise its cases. Neither is
counted as successful validation; the final runner corrects both and retains the
original logs alongside the complete passing reruns.

Run the private target reproduction from the repository:

```powershell
wsl -d Ubuntu --cd /mnt/c/code/sloop-fm1/build/ui-reuse-98/layers/final-source --exec env FELUCCA_AUDIO_DIAG=0 JIELI_TOOLCHAIN=/home/shaun/.cache/sloop-host-tools/jieli-linux-toolchains-20250324.1 python3 build/build-target.py
wsl -d Ubuntu --cd /mnt/c/code/sloop-fm1 --exec python3 build/ui-reuse-98/layers/final-checks.py
wsl -d Ubuntu --cd /mnt/c/code/sloop-fm1 --exec python3 build/ui-reuse-98/layers/menu-checks.py
```

The retained harnesses use pinned Zig 0.13, `-UNDEBUG -O2 -fwrapv
-fno-strict-aliasing`. Host equality does not measure physical frame duration,
both cores' stack high-water, or worst-case audio deadlines. No audio golden was
updated; this candidate does not change musical or DSP code.
