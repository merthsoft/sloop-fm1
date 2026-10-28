#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Protect target LOFI sample loops from division/call regressions.

The control setup may divide. This is an instruction-structure check, not a
cycle estimator: conditionally skipped slow paths distort nested-span costs.
"""
import sys
from target_budget import FUNCS, functions, cost

FUNCS.append('vinyl_process_mode')
path=sys.argv[1] if len(sys.argv)>1 else 'build/felucca.dis'
body=functions(path).get('vinyl_process_mode')
assert body, 'LOFI renderer missing from target disassembly'
result=cost(body)
assert result['div']==0, result
assert result['call']==0, result
print('Target master LOFI: zero hardware divides and zero helper calls in sample loops PASS')
