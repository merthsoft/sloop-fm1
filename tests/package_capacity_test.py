# SPDX-License-Identifier: GPL-3.0-only
"""Exercise the real encrypted package at its largest supported app extent."""
from pathlib import Path
import struct
import sys

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'tools'))
import fm1pkg_make as pkg

pkg.SDK = sys.argv[1] if len(sys.argv) > 1 else None
app = (bytes(range(256)) * ((pkg.APP_SLOT + 255) // 256))[:pkg.APP_SLOT]
flash = pkg.flash_image(app, pkg.KEY)
assert len(flash) == pkg.FLASH_SIZE == 0x93000
region = bytearray(flash[0x4000:])
pkg.sfc(region, 0, len(region), 0, pkg.KEY)

def check_entry(at):
    e = region[at:at + 32]
    hcrc, dcrc, off, size = struct.unpack_from('<HHII', e)
    assert hcrc == pkg.crc16(e[2:])
    return dcrc, off, size

crc, off, size = check_entry(0)
assert off == 0x02000120 and pkg.crc16(region[32:size]) == crc
crc, off, size = check_entry(32)
assert off == 0x120 and size == len(app)
assert region[off:off + size] == app
assert pkg.crc16(app) == crc
crc, cfg_off, cfg_size = check_entry(64)
assert cfg_off == off + size
assert region[cfg_off:cfg_off + cfg_size] == pkg.sdk_file('cfg_tool.bin')
assert pkg.crc16(region[cfg_off:cfg_off + cfg_size]) == crc

try:
    pkg.flash_image(app + bytes(4), pkg.KEY)
except SystemExit:
    pass
else:
    raise AssertionError('one word beyond capacity accepted')

# An SDK change must fail rather than silently move the config past the updater.
sdk_file = pkg.sdk_file
pkg.sdk_file = lambda rel: sdk_file(rel) + (bytes(4) if rel == 'cfg_tool.bin' else b'')
try:
    pkg.flash_image(app, pkg.KEY)
except SystemExit as error:
    assert 'SDK config payloads' in str(error)
else:
    raise AssertionError('larger SDK config accepted without a slot review')
print('package capacity: maximum extent, encrypted data/CRCs, oversize and SDK drift PASS')
