# Integrated flash savings from the .91 audits

The UI, storage and two no-additional-RAM startup-data candidates are integrated
in firmware 2.8.0.92. DSP outlining, compiler-option changes, larger literal RAM
allocations, CRC slowdown and all rejected experiments remain excluded.

## Actual combined measurements

All staged measurements use normal production firmware, the pinned JieLi
toolchain, existing -Oz and the real 584,336-byte application limit. No region,
reserve, feature, sound quality, protection or upstream format was removed.

| Linked source | Image bytes | Flash free | Saving against frozen .91 |
| --- | ---: | ---: | ---: |
| Frozen .91 baseline | 584,296 | 40 | — |
| UI combination, still .91 | 582,808 | 1,528 | 1,488 |
| UI + storage, still .91 | 582,536 | 1,800 | 1,760 |
| UI + storage + startup data, still .91 | 582,320 | 2,016 | 1,976 |
| Released version string .92 | **582,324** | **2,012** | **1,972** |

These are actual sequential full links, not added standalone estimates. The
version string changes compressed literal size/layout by four bytes. Main static
RAM remains **97,500 / 98,304 B (804 free)**, low data **28,928 / 29,392 B
(464 free)**, and pool **335,424 / 344,064 B**, with the 8,192-byte reserve
intact and 448 B beyond it. Worker RAM code remains 160 B and flash-write RAM
code remains 925 instructions without calls.

## Integrated changes and costs

- Small-font/palette text, common rectangle/line colors and full-width canvas
  operations share small cold UI wrappers. Palette reads remain dynamic, and
  layout, clipping, return positions and input/action ownership are unchanged.
  Non-tail wrappers add a call and their own approximately 12-byte activation;
  band wrappers tail-branch. This is a flash reduction, not a proven UI speedup
  or device stack high-water result.
- FUN4/FUN5 track prefixes and FUN2/FUN3 conversion bodies share implementations;
  FUN1 defaults and format-specific validation/remapping remain explicit.
  Storage retains forward-copy semantics, and editor signatures reuse the same
  FNV recurrence. Cold helper paths add small call/stack costs documented in
  the [storage audit](FLASH-91-STORAGE-AUDIT.md). Persistence buffers, A/B commit
  ordering, snapshot associations and recovery lifetimes are unchanged.
- The drum generator deterministically places similar immutable dictionary rows
  together, remapping every factory-kit index. It starts with the original row
  zero, retains all 242 rows and all 512 original sound selections, and uses the
  existing direct runtime lookup after boot. No runtime remap or decoder is added.
- The startup literal grammar can also use unused control bytes 1–31 after its
  high-byte token range. NUL and every byte already present in input remain
  literals. Encoder and C decoder share the index convention; the existing
  bounded expansion stack, restored destinations and early initialization phase
  remain. Decoder range checks add startup work only.

No playback/sample decoding, overload threshold, voice admission, worker protocol,
render split or effect algorithm is changed by this batch.

## Validation and retained evidence

The combined integration is tested through actual production UI, storage and
factory-lookup functions. New maintained checks are `ui_gfx_sharing_test.c` and
`drum_factory_identity_test.c`; the latter uses the independently frozen original
11,264-byte fixture under `tests/fixtures/`. Existing persistence fault tests gain
640,000 editor-signature comparisons against the previous recurrence. Python/C
literal checks cover original low/high bytes, nested control tokens, NUL, cyclic
trees, guard bytes, deterministic encoding and invalid token budgets.

The integration also runs UI navigation/fuzz and original framebuffer comparison,
legacy conversion/import/rejection corpus, persistence/partial-write/snapshot
checks, MIDI/scale/note ownership, multicore and timeout recovery, audio goldens,
target loop budgets/inlining, complete target literal byte/address restoration,
worker HAL injection and browser/native/installer checks. Individual logs and
staged links remain under `build/flash-audit-91/integration/`.

Final results: all 200 audio goldens are unchanged; all 117 original framebuffer
captures are present, with 114 byte-identical and only three version-label
captures differing within the final digit's pixels. The 42,000-case legacy
conversion corpus retains digest `8b386c47`. Factory PCM/ADPCM storage remains
303,054 byte-identical bytes. All selected regression programs and dual-core/
timeout variants pass, as do six focused UBSan programs, browser/native parity,
16 deployment checks and exact local package readback. Focused fixed-point host
sanitizer runs exclude inherited signed-shift checks; actual target literal C
restoration and HAL injection use nonrecovering UBSan without that exclusion.

Host pixel/audio/state equality and target structure checks do not qualify
physical UI responsiveness, cold/warm/recovery boot duration, watchdog margin,
both cores' stack high-water or worst-case audio deadlines. Those remain device
checks, especially while recording and using USB, heavy LOFI and visualizers.

The [audit synthesis](FLASH-91-AUDIT-SYNTHESIS.md) preserves the independently
measured alternatives and the remaining DSP/compiler experiments.
