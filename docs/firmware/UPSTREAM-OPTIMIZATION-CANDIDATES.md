# Optimization suggestions for upstream

Source checked against fetched `upstream/main` **ec9e537**. No PR or issue was
created. Measurements below are controlled **MerthSloop** comparisons, not
measurements of an upstream build; do not promise the same deltas upstream.

## Best first contribution: mostly-zero runtime state in BSS

Upstream still initializes the complete drum runtime object with `.set = -2`
and both limiter objects with `.env = LIM_T, .g = 32768`. Nonzero initialization
puts their zero-filled audio/state buffers into the firmware's load image too.

Our changes separate the small nonzero drum lookup sentinel and use existing
first-use semantics for limiter control values, so the large objects are zeroed
at startup instead of stored in flash. The controlled branch audit measured:

| Change | Saving alone |
| --- | ---: |
| Drum state/cache split | 2,032 B |
| Limiter buffers to BSS | 780 B |
| Both together | **2,796 B** |

The combined delta is not the sum because compiler layout changes interact.
Static RAM did not increase in that comparison. Drum rendering adds no work;
the limiter uses a first-use check once per block, not per sample. Output
equivalence and unchanged golden renders were checked. See the
[frozen experiment](PIANO-ROLL-SPACE-AUDIT.md) for exact sources and limitations.

The texture state has a similar pattern: its reset already supplies its PRNG
seed, making a nonzero aggregate initializer unnecessary. That branch
experiment saved 448 B alone. It is a useful separate patch after checking the
upstream reset/call ownership and measuring their current compiler output.

Suggested upstream workflow: measure their normal release before/after; compare
first-use/reset limiter output and drum/texture behavior; keep each change
independently reviewable. This is the lowest-complexity recommendation, without
asset compression or changes to the flash partition.

## Good second contribution: startup-only immutable metadata packing

Factory presets and certain constant tables can use a compact flash
representation, restored before audio/core 1 starts. Playback still reads the
ordinary records directly. The preset experiment saved **2,580 B net**, including
decoder/accessor overhead, at a cost of **5,508 B persistent low RAM**. All 153
records and 182 actual-restoration golden renders matched. Hardware cold-start
duration/watchdog margin remains unmeasured.

This needs more review than BSS cleanup: two-pass object-derived generation,
an explicit record ABI, bounded startup scratch and available low RAM. New
presets remain ordinary C initializers; the build extracts them rather than
requiring hand-maintained compressed records. Do not describe it as a free
flash saving or per-hit audio decompression. Numeric/FIR table packing can be
proposed separately with its own build measurement and startup/RAM accounting.

## Other possibilities and exclusions

- **Shared cold UI paths:** the duplication audit measured 256 B combined from
  shared entry setup and parameter-card rendering. The entry helper is fairly
  portable; the card patch depends on our UI and should be reimplemented against
  upstream rather than cherry-picked wholesale. See [audit](REDUNDANCY-AUDIT.md).
- **Immutable browser indices:** our branch measured 720 B alone and 160 B RAM,
  but upstream's current preset bank/order must be checked. Generated/validated
  indices are essential so factory reordering cannot break favourites or MIDI
  program selection.
- **Extra 2,772 B app capacity:** a package-layout change, not removal of code.
  It uses trailing space within the unchanged update boundary with our exact SDK
  config sizes. Recommend only after their SDK/package arithmetic and physical
  SPL/update/return-to-stock validation; not the first optimization PR.
- **Compiler `-Oz`:** potentially useful with explicit hot helper inlining and
  target call/cost checks. Do not recommend a blind global flag change based
  only on smaller output.
- **Font packing:** upstream already adopted packed fonts. Do not propose the
  original change again or quote our obsolete font delta as a new saving.
- **Factory sample compression:** do not recommend our withdrawn experiment.
  The user noticed timing/load sensitivity on sample-heavy arrangements. Raw
  factory sample reads are intentionally retained.

Historical deltas have different baselines and must not be added into one
claimed upstream total. A future PR should include upstream's exact toolchain,
build flags, before/after app bytes, RAM occupancy, output checks and any new
runtime cost. No upstream measurements or publication are implied here.
