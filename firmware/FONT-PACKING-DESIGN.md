# Lossless font packing

The Terminus BDF font has binary pixel coverage. The generated small font previously
stored each pixel in a four-bit alpha slot. `tools/gen_font.py` now packs those binary
pixels into one-bit rows and writes an explicit `FONT_DATA_BITS=1` marker.
`src/gfx.c` decodes that format locally; an unmarked old generated header keeps its
previous four-bit decoding. Other font formats retain their alpha coverage.

All 224 glyphs, metrics, padding and character coverage remain unchanged. Large text
still reuses the small font at 2x. Bitmap data shrinks from 21,504 to 5,376 bytes:
16,128 bytes saved before decoder/compiler changes. The splash remains present.

`tests/font_asset_test.py` compares generated glyphs against an independent BDF oracle.
`tests/font_render_test.c` exercises the actual renderer for every glyph, colors,
scaling and clipped positions: 6,120 comparisons for each of packed and legacy assets.

The optimization stays in the generator and bitmap renderer, avoiding changes to
audio, sample codecs, persisted data and the linker. During upstream merges preserve
the format marker and its compatible decoder together; regenerate assets and rerun
the pixel tests. Do not infer whole-image savings from the bitmap count: target builds
also include new functionality and decoder changes.
