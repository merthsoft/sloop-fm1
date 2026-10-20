# Firmware static-analysis audit

The audit covers the production firmware translation unit and selected build
variants. It distinguishes source that is unused in one configuration from
code/data that still occupies memory or executes at runtime.

## Findings and applied cleanup

| Finding | Change | Measured result |
| --- | --- | --- |
| Right-channel scope samples were written by the audio ISR and copied by the UI, but no retained visualizer read them. | Remove the right ring and UI snapshot; preserve the left ring and stereo audio/USB paths. | 1,024 bytes of declared RAM removed, fewer ISR stores and UI copies. |
| Retired visualizers left write-only volatile note/kick flags and last-note bytes in note-on. | Remove those globals and writes; retain note_hits/vis_note_hits used by key LEDs and Note Trails. | Six declared bytes removed and less per-note work. |
| Four orphan drawing helpers had no callers. | Remove vis_circle, vis_poly, vis_seg and vis_position. | Source cleanup only: the compiler already omitted them from the image. |
| RAM-only builds referenced ST_SIDE_MAX, defined only with flash storage. | Guard the storage-capacity assertion with FELUCCA_FLASH; retain the layout assertion in every configuration. | RAM-only static analysis now compiles; production layout unchanged. |

Against the saved production baseline, the cleanup alone changed image size
576,832 → 576,800 bytes and main static RAM 82,632 → 81,592 bytes. The total RAM
recovery is **1,040 bytes**, including layout/alignment effects. The history union
remains 2,400 bytes because Note Trails, rather than the waveform, sets its size.
Low RAM and the audio pool are unchanged.

Target ISR instructions fall from 234 to 230; its weighted loop cost falls from
268 to 256, with three loop call sites unchanged. This is a static compiler
metric, not measured device CPU percentage or a claim about deadline latency.
All 182 audio goldens and all seven visualizer framebuffer captures are unchanged.

The subsequent UI-consistency pass removes the provisional FM6 diagrams,
consolidates knob-card drawing and refreshes SONG/recording layouts. Its current
combined build is **574,856 image bytes, 6,708 free, 81,592 static RAM bytes**.
That separate UI delta must not be attributed to dead-code removal alone.

## Unused in production does not always mean obsolete

The first compiler warning pass found 30 unused functions. Four obsolete drawing
helpers were removed; 26 remain. Their absence from the production symbol table
means deleting them would not reclaim additional production flash.

- Test/adaptor helpers: arp_list, arp_next, sequence_starter_step,
  sequence_starter_voiced, sequence_starter_apply, mi_row, up_pat_empty,
  fm6_bank_has and note_set_len. Their callers include regression tests and old
  capture fixtures; the production paths use their more expressive counterparts.
- Optional CDC/debug helpers: usb_cdc_now, cpu1_stack_free, lowram_state,
  stack_free_main, stack_free_sup and stack_sp. Keep the supported diagnostic
  configuration working.
- HAL/alternate entry points: fm1_wdt_stop, fm1_mem_readable, fm1_irq_mask,
  fm1_guard_top_locked, fm1_guard_unlock_top, fl_erase4k and fl_verify_xip.
  Removing upstream helpers solely to silence default-build warnings increases
  merge work without an image-size benefit.
- Other currently omitted helpers: div_samples, ds_on, panel_led and seq_is_empty.
  These are cleanup candidates if their APIs are intentionally retired, with
  no measured production flash gain available today.

## Remaining optimization candidates

These are opportunities to profile, not approved savings estimates:

1. HOME hashes all stored steps for all four tracks on each UI check, even though
   it displays one 16-step bank. A visible-bank checksum or revision cache could
   reduce UI work, but every edit/restore path must invalidate it correctly.
2. The display folds lowercase glyphs to uppercase, while te_lower converts
   many labels to lowercase first. Removing the conversion may reduce cold UI
   work, but text-based tests and cache signatures require review.
3. graph_big still duplicates knob values on pages without a useful schematic.
   A later engine-graphics pass should replace that path where appropriate,
   rather than retain two complete renderers for the same page.
4. The factory sample payload is 303,054 bytes, over half the current image.
   It is reachable instrument data, not dead code. Runtime decompression was
   deliberately removed after hardware feedback; do not bring it back merely
   to advertise a space saving.

No per-sample DSP outlining, sample recompression, instrument removal or
normalization/gain changes are part of this audit.

## Reproduce

After generating the normal build headers, run in the Linux target-toolchain
environment:

```sh
JIELI_TOOLCHAIN=/path/to/jieli-toolchain python3 tools/analyze_firmware.py
```

The tool reads build/gen without regenerating it. Logs, exact commands, the
unused-function inventory and JSON results go under build/static-audit/current.
It runs unused/reachability warnings and Clang core/dead-store checks against
production, CDC, single-core and RAM-only/OTA-off configurations. A deliberately
invalid null-pointer probe must produce a diagnostic first, preventing a broken
analyzer invocation from being reported as a clean scan.

All four analyzed configurations complete with no analyzer diagnostics. The
production warning pass reports the 26 intentionally triaged unused functions;
no unreachable-statement or unused-variable warnings were emitted. Static
analysis is not proof against IRQ/core races, timing faults, hardware behavior
or every indirect-call/input-dependent path. Audio regressions, renderer tests,
USB tests and target budget/inlining checks complement it.
