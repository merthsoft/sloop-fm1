# SPDX-License-Identifier: GPL-3.0-only
"""Numerical checks for the non-mutating preset loudness audit."""
import sys
import unittest
from pathlib import Path

import numpy as np

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'tools'))
from audit_preset_loudness import FS, integrated


class LoudnessTest(unittest.TestCase):
    def setUp(self):
        self.tone = .1 * np.sin(2 * np.pi * 1000 * np.arange(4 * FS) / FS)

    def test_stereo_energy_and_polarity(self):
        mono = integrated(self.tone)
        stereo = integrated(np.column_stack([self.tone, self.tone]))
        opposite = integrated(np.column_stack([self.tone, -self.tone]))
        self.assertAlmostEqual(stereo - mono, 10 * np.log10(2), places=9)
        self.assertAlmostEqual(opposite, stereo, places=9)

    def test_gain_and_absolute_calibration(self):
        # A -20 dBFS peak 1 kHz sine is approximately -23 LUFS, mono.
        self.assertAlmostEqual(integrated(self.tone), -23, delta=.1)
        self.assertAlmostEqual(integrated(self.tone / 2) - integrated(self.tone),
                               -20 * np.log10(2), places=8)

    def test_silence_and_release_gate(self):
        self.assertTrue(np.isnan(integrated(np.zeros(FS))))
        with_tail = np.concatenate([self.tone, np.zeros(4 * FS)])
        self.assertAlmostEqual(integrated(with_tail), integrated(self.tone), delta=.3)


if __name__ == '__main__':
    unittest.main()
