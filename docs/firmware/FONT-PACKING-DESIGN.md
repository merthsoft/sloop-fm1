# Lossless font packing

The Terminus BDF font has binary pixel coverage. The generated small font previously
stored each pixel in a four-bit alpha slot. `tools/gen_font.py` now packs those binary
pixels into one-bit rows and writes an explicit `FONT_DATA_BITS=1` marker.
`src/gfx.c` decodes that format locally; an unmarked old generated header keeps its
previous four-bit decoding. Other font formats retain their alpha coverage.

All 224 glyphs, metrics, padding and character coverage remain unchanged. Large text
still reuses the small font at 2x. Upstream now includes one-bit bitmap packing:
21,504 to 5,376 bytes. This fork additionally shares identical packed glyphs through
the existing offset table, then exact byte-aligned suffix/prefix overlap, reducing
data to 3,952 bytes (1,424 more saved). Each complete glyph remains a contiguous,
unchanged sequence, including raw lower-case and Latin-1 bitmaps. Drawing
uses the same renderer with no extra decoder or runtime lookup. The current fork
uses a text startup title rather than the splash bitmap.

`tests/font_asset_test.py` compares generated glyphs against an independent BDF oracle.
`tests/font_render_test.c` exercises the actual renderer for every glyph, colors,
scaling and clipped positions: 6,120 comparisons for each of packed and legacy assets.

The optimization stays in the generator and bitmap renderer, avoiding changes to
audio, sample codecs, persisted data and the linker. During upstream merges preserve
the format marker and its compatible decoder together; regenerate assets and rerun
the pixel tests. Do not infer whole-image savings from the bitmap count: target builds
also include new functionality and decoder changes.
