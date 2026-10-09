#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Generate the binary-font renderer regression fixtures.

Run on the build host: python tests/font_asset_test.py
Then compile tests/font_render_test.c with -Ibuild/host/font-assets and -Ifirmware/src.
The reference contains uncompressed BDF pixels, independent of either font decoder.
"""
import contextlib
import importlib.util
import io
import re
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
out = ROOT / "build/host/font-assets"
out.mkdir(parents=True, exist_ok=True)
try:
    import PIL
except ImportError:
    # Firmware generation already requires Pillow, but headless WSL tests can
    # consume the resulting production header without installing image tools.
    (out / "felucca_font.h").write_text((ROOT / "build/gen/felucca_font.h").read_text())
else:
    spec = importlib.util.spec_from_file_location("gen_font", ROOT / "tools/gen_font.py")
    gen = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(gen)
    with contextlib.redirect_stdout(io.StringIO()):
        gen.main(out / "felucca_font.h")

# Independent BDF oracle. Check the font geometry explicitly instead of sharing
# the generator's rendering/packing implementation with its regression test.
bdf = (ROOT / "assets/fonts/ter-u16n.bdf").read_text().splitlines()
assert "FONTBOUNDINGBOX 8 16 0 -4" in bdf
height = 16
by_code = {}
for record in "\n".join(bdf).split("STARTCHAR ")[1:]:
    lines = record.splitlines()
    code = int(next(v for v in lines if v.startswith("ENCODING ")).split()[1])
    width, rows, left, bottom = map(int, next(v for v in lines if v.startswith("BBX ")).split()[1:])
    advance = int(next(v for v in lines if v.startswith("DWIDTH ")).split()[1])
    first = lines.index("BITMAP") + 1
    pixels = [0] * 192
    for y, bits in enumerate(lines[first:first + rows]):
        value = int(bits, 16)
        for x in range(width):
            px, py = left + x, 12 - bottom - rows + y
            if 0 <= px < 8 and 0 <= py < 16 and value & (1 << (((width + 7) // 8) * 8 - 1 - x)):
                pixels[py * 12 + px + 2] = 15
    by_code[code] = (advance, 12, pixels)
glyphs = [by_code.get(code, by_code[63]) for code in range(32, 256)]
header = (out / "felucca_font.h").read_text()
assert "#define FONT_DATA_BITS 1" in header, "regenerate firmware font assets before testing"
data = bytes(int(v, 16) for v in re.findall(r"0x([0-9a-f]{2})", re.search(
    r"FONT_S_DATA\[.*?\] = \{(.*?)\};", header, re.S).group(1)))
offsets = list(map(int, re.search(r"FONT_S_OFF\[.*?\] = \{(.*?)\};", header, re.S).group(1).split(",")))
assert len(data) == 5376, len(data)
for i, (_, width, pixels) in enumerate(glyphs):
    actual = [15 if data[offsets[i] + p // 8] & (1 << (7 - p % 8)) else 0 for p in range(width * height)]
    assert actual == pixels, f"packed glyph {i + 32} differs from BDF"
ref = ["/* Generated test oracle: original unpacked BDF glyph pixels. */",
       "static const unsigned char REF_ADV[] = {" + ",".join(str(a) for a, _, _ in glyphs) + "};",
       "static const unsigned char REF_PIX[][192] = {"]
ref += ["{" + ",".join(str(v) for v in pixels) + "}," for _, _, pixels in glyphs]
ref += ["};"]
(out / "font_reference.h").write_text("\n".join(ref))

# Exercise old-header compatibility in the same C renderer test: reconstruct
# the previous four-bit rows directly from the original BDF, without a marker.
legacy = bytearray()
for _, width, pixels in glyphs:
    for row in range(height):
        for col in range(0, width, 2):
            legacy.append((pixels[row * width + col] << 4) | pixels[row * width + col + 1])
legacy_header = re.sub(r"#define FONT_DATA_BITS 1\n", "", header)
legacy_header = re.sub(r"FONT_S_DATA\[.*?\] = \{.*?\};", "FONT_S_DATA[21504] = {" +
                       ",".join(str(v) for v in legacy) + "};", legacy_header, flags=re.S)
legacy_header = re.sub(r"FONT_S_OFF\[.*?\] = \{.*?\};", "FONT_S_OFF[224] = {" +
                       ",".join(str(i * 96) for i in range(224)) + "};", legacy_header, flags=re.S)
old = out / "legacy"
old.mkdir(exist_ok=True)
(old / "felucca_font.h").write_text(legacy_header)
print("font assets: all 224 BDF glyphs identical; 21,504 -> 5,376 bytes (16,128 saved)")
