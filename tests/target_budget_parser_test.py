#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Keep target loop guards independent of cold-block placement."""
import io
import unittest
from unittest.mock import patch

from target_budget import cost, cyclic_heads, functions


def branch(address):
    return f"goto -2 <fixture : {address:x} >"


class TargetLoops(unittest.TestCase):
    def test_cold_rejoin_is_not_a_loop(self):
        code = [(0x10, "if (r0 == 0) goto 12 <fixture : 20 >"),
                (0x12, "call 8 <hot : 40 >"), (0x14, "rts"),
                (0x20, "call 8 <cold : 50 >"), (0x22, branch(0x12))]
        self.assertEqual(cyclic_heads(code), {})
        self.assertEqual(cost(code)["call"], 0)

    def test_nested_real_loops_keep_weights_and_calls(self):
        code = [(0x10, "r0 = 0"), (0x12, "call 8 <helper : 40 >"),
                (0x14, "if (r0 != 0) " + branch(0x12)),
                (0x16, "if (r1 != 0) " + branch(0x10)), (0x18, "rts")]
        self.assertEqual(cyclic_heads(code), {0x12: 0x14, 0x10: 0x16})
        self.assertEqual(cost(code), {"insns": 5, "loop": 4, "div": 0,
                                     "call": 1, "cost": 10})

    def test_early_return_does_not_erase_a_loop(self):
        code = [(0x10, "goto 12 <fixture : 20 >"), (0x12, "rts"),
                (0x20, "call 8 <helper : 40 >"),
                (0x22, "if (r0 != 0) " + branch(0x10)), (0x24, "rts")]
        self.assertEqual(cyclic_heads(code), {0x10: 0x22})
        self.assertEqual(cost(code)["call"], 1)

    def test_divide_still_counts_in_a_real_loop(self):
        code = [(0x10, "r0 = r1 / r2 (u)"),
                (0x12, "if (r0 != 0) " + branch(0x10)), (0x14, "rts")]
        self.assertEqual(cost(code)["div"], 1)
        self.assertEqual(cost(code)["cost"], 10)

    def test_computed_control_flow_remains_conservative(self):
        for jump in ("goto r0", "if (predicated) goto r0", "pc = r0"):
            code = [(0x10, jump), (0x12, "rts"), (0x20, "call 8 <h : 40 >"),
                    (0x22, branch(0x10))]
            self.assertEqual(cyclic_heads(code), {0x10: 0x22})

    def test_predicated_return_and_goto_retain_false_path(self):
        for instruction in ("rts", "{pc, r4} = [sp++]", "goto 12 <f : 30 >"):
            source = ("fm1_alnk0_irq:\n"
                      " 10: 00 00  if (r0 != 0) {\n"
                      f" 12: 00 00      {instruction}\n"
                      "                 }\n"
                      " 14: 00 00  call 8 <h : 40 >\n"
                      " 16: 00 00  if (r1 != 0) goto -8 <f : 10 >\n"
                      " 18: 00 00  rts\n"
                      " 30: 00 00  rts\n")
            with patch("builtins.open", return_value=io.StringIO(source)):
                code = functions("fixture.dis")["fm1_alnk0_irq"]
            self.assertTrue(code[1][1].startswith("if (predicated)"))
            self.assertEqual(cyclic_heads(code), {0x10: 0x16})
            self.assertEqual(cost(code)["call"], 1)


if __name__ == "__main__":
    unittest.main()
