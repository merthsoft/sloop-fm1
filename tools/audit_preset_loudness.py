#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Read tests/preset_loudness.c output without changing factory trims.

Usage: python tools/audit_preset_loudness.py RAW_DIRECTORY PRESET_LIST OUTPUT.csv
PRESET_LIST is argv[2] from tests/preset_level.c. Needs numpy and scipy.
LUFS uses summed K-weighted stereo energy (not a mono fold). RMS is mean
channel energy, dBFS; peak is a sample peak, not a true-peak measurement.
"""
import csv
import sys
from pathlib import Path

import numpy as np
from scipy.signal import lfilter

FS = 44100


def integrated(x):
    """BS.1770-style stereo integrated loudness, absolute and relative gates."""
    x = np.asarray(x, dtype=float)
    if x.ndim == 1:
        x = x[:, None]
    # BS.1770 K weighting designed for 44.1 kHz. The legacy trim generator
    # uses the 48 kHz reference coefficients despite rendering at 44.1 kHz.
    k = np.tan(np.pi * 1681.974450955533 / FS)
    q = .7071752369554196
    vh = 10 ** (3.999843853973347 / 20)
    vb = vh ** .4996667741545416
    a0 = 1 + k / q + k * k
    shelf_b = np.array([vh + vb*k/q + k*k, 2*(k*k-vh), vh-vb*k/q+k*k]) / a0
    shelf_a = [1, 2*(k*k-1)/a0, (1-k/q+k*k)/a0]
    y = lfilter(shelf_b, shelf_a, x, axis=0)
    k = np.tan(np.pi * 38.13547087602444 / FS)
    q = .5003270373238773
    a0 = 1 + k / q + k * k
    y = lfilter([1, -2, 1], [1, 2*(k*k-1)/a0, (1-k/q+k*k)/a0], y, axis=0)
    energy = np.sum(y * y, axis=1)
    w, h = int(.4 * FS), int(.1 * FS)
    blocks = np.array([energy[i:i+w].mean() for i in range(0, len(x)-w+1, h)])
    absolute = blocks[blocks > 10 ** ((-70 + .691) / 10)]
    if not len(absolute):
        return float('nan')
    gated = absolute[absolute > absolute.mean() / 10]
    return float(-.691 + 10 * np.log10(gated.mean()))


def db(x):
    return float(20 * np.log10(max(float(x), 1e-12)))


def main(directory, presets, output):
    rows = []
    for line in Path(presets).read_text().splitlines():
        e, p, role, name = line.split(None, 3)
        for mode in range(4):
            path = Path(directory) / f'{e}-{p}-{mode}.raw'
            x = np.fromfile(path, dtype='<i4').reshape(-1, 2) / 32768.
            assert len(x) == (4 * FS // 32) * 32, path
            rows.append(dict(engine=e, preset=p, name=name,
                             stimulus=['C3', 'C4', 'C5', 'Cmaj7'][mode],
                             lufs=integrated(x), mono_lufs=integrated(x.mean(axis=1)),
                             peak_dbfs=db(np.abs(x).max()),
                             attack_rms=db(np.sqrt(np.mean(x[:FS//4] ** 2))),
                             sustain_rms=db(np.sqrt(np.mean(x[FS:3*FS] ** 2))),
                             release_rms=db(np.sqrt(np.mean(x[3*FS:] ** 2)))))
    with Path(output).open('w', newline='') as f:
        writer = csv.DictWriter(f, fieldnames=rows[0])
        writer.writeheader()
        writer.writerows(rows)
    print(f'{len(rows)} controlled renders -> {output}')


if __name__ == '__main__':
    main(*sys.argv[1:])
