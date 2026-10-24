#!/usr/bin/env python3
"""Representation checks for font metrics and comment-preserving preset trims."""
import contextlib
import hashlib
import importlib.util
import io
import sys
import tempfile
from pathlib import Path
import re
import unittest
from unittest.mock import patch, MagicMock

ROOT = Path(__file__).resolve().parents[1]


def load(name):
    spec = importlib.util.spec_from_file_location(name, ROOT / "tools" / (name + ".py"))
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


font = load("gen_font")
trim = load("level_presets")


class Representations(unittest.TestCase):
    def test_metric_selection(self):
        for field in ["ADV", "BW"]:
            lines = []
            self.assertEqual(font.emit_metric(lines, "T", field, [12, 12]), ("0", 12))
            self.assertEqual(lines, [])
            self.assertEqual(font.emit_metric(lines, "T", field, [7, 9]), ("FONT_T_" + field, 0))
            self.assertIn("{7, 9}", lines[0])
        for values in [[], [-1], [256]]:
            with self.assertRaises(ValueError):
                font.emit_metric([], "T", "ADV", values)

    def test_real_font_generation(self):
        with patch.object(Path, "write_text") as write, contextlib.redirect_stdout(io.StringIO()):
            font.main("font.h")
            header = write.call_args.args[0]
        self.assertNotRegex(header, r"FONT_[SL]_(ADV|BW)\[")
        self.assertIn("FONT_S_DATA, 1, 8, 12", header)
        self.assertIn("FONT_S_DATA, 1, 16, 24", header)

    def test_proportional_font_generation(self):
        glyphs = [(7, 3, [15, 0, 15]), (9, 4, [15, 15, 0, 15])]
        with patch.object(Path, "write_text") as write, contextlib.redirect_stdout(io.StringIO()), \
             patch.object(font, "SIZES", [("T", "synthetic", 1, 1, False, 48, 49)]), \
             patch.object(font, "render", return_value=(1, glyphs)):
            font.main("font.h")
            header = write.call_args.args[0]
        self.assertIn("FONT_T_ADV[2] = {7, 9}", header)
        self.assertIn("FONT_T_BW[2] = {3, 4}", header)
        self.assertIn("FONT_T_ADV, FONT_T_BW, FONT_T_OFF, FONT_T_DATA, 1, 0, 0", header)

    def test_trim_holes_and_empty_rows(self):
        header = trim.format_header([[1, 0, -3, 0], [0] * 4], [["A", "", "B", ""], [""] * 4])
        self.assertIn("PT_OFFSET[] = {0, 3}", header)
        self.assertIn("PT_COUNT[] = {3, 0}", header)
        self.assertIn("/* 0.1 - */ 0", header)
        self.assertIn("/* 0.2 B */ -3", header)
        self.assertIn("PRESET_TRIM[] = {\n    0", trim.format_header([[0]], [[""]]))
        for tab, names in [([], []), ([[1]], [[]]), ([[128]], [["A"]])]:
            with self.assertRaises(ValueError):
                trim.format_header(tab, names)

    def test_current_trim_roundtrip(self):
        header = (ROOT / "firmware/src/preset_trim.h").read_text()
        entries = re.findall(r"/\* (\d+)\.(\d+) (.*?) \*/ (-?\d+)", header)
        tab = [[0] * 32 for _ in range(13)]
        names = [[""] * 32 for _ in range(13)]
        for e, p, name, value in entries:
            tab[int(e)][int(p)] = int(value)
            names[int(e)][int(p)] = name
        self.assertEqual(header, trim.format_header(tab, names))
        # Freeze the original gain table; appended preset trims have their own
        # measured values and must not mask accidental recalibration of old sounds.
        original_counts = (22, 22, 8, 8, 11, 7, 14, 8, 8, 8, 26, 11, 4)
        for e, row in enumerate(tab):
            row[original_counts[e]:] = [0] * (32 - original_counts[e])
        raw = bytes(value & 255 for row in tab for value in row)
        self.assertEqual(hashlib.sha256(raw).hexdigest(), TRIM_SHA256)
        self.assertEqual(len(entries), 175)

    def test_new_only_preserves_old_and_unrendered_engine(self):
        # A production no-SLICE render must not delete the optional trim row or
        # change existing gains while a new instrument is being calibrated.
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            header, report = root / "trim.h", root / "report.txt"
            header.write_text(trim.format_header([[7, 0], [-4, 0]], [["OLD", ""], ["OPTIONAL", ""]]))
            report.write_text("0 0 3 OLD\n0 1 3 NEW\n")
            fake_numpy = MagicMock()
            with patch.dict(sys.modules, {"numpy": fake_numpy}), patch.object(trim, "OUT", header), \
                 patch.object(trim, "lufs", return_value=-20.0), contextlib.redirect_stdout(io.StringIO()):
                trim.main(root / "fake.raw", report, new_only=True)
            result = header.read_text()
            self.assertIn("/* 0.0 OLD */ 7", result)
            self.assertIn("/* 0.1 NEW */ 10", result)
            self.assertIn("/* 1.0 OPTIONAL */ -4", result)


TRIM_SHA256 = "5bfcb5eb188210644675ee6e4e7390ea4b1ec49ae9ef96fccad4712c6de1d5a4"

if __name__ == "__main__":
    unittest.main()
