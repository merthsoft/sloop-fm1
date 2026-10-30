#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
# Copyright (C) 2026 Leo Kuroshita (@kurogedelic), Hügelton Instruments
"""Target-side cost estimate of the hot DSP code, from the pi32v2 objdump listing tools/build.py writes
(build/felucca.dis). For each engine's render function and the audio ISR: the instructions
inside loops (cyclic backward branches: the per-sample loops and what they
contain), and the hardware divides and calls in them. cost = the loop instructions, each weighted 4 per
level of nesting (an inner loop runs several times per sample) and 1 + 8 for a divide (many cycles). A static count, not a cycle count: it changes only when the compiled code changes,
so it is exact run to run, and it catches a render loop that grew (more work per sample, a new divide,
something not inlined any more). Compared with tests/target_budget.txt at +10 %. The host
CPU check (regress.c) is the primary one; this one sees the target compiler's code.
  tests/target_budget.py [DIS [BUDGET]]      BUDGET_UPDATE=1 rewrites BUDGET"""
import os
import re
import sys

FUNCS = ["analog_render", "analog_body", "digital_render", "phase_render", "lofi_render", "sample_render", "formant_render",
         "trio_render", "trio_pass", "drawbar_render", "drawbar_block",
         "grain_render", "grain_block", "slicer_track", "drums_mix",
         "phys_render", "noise_render", "px_modal_run", "px_string_run", "px_string_excite", "px_symp_run",
         "fm6_op_run", "fm6_op_fb", "fm6_render",     # FM6 (SLOOP 2.4): the operator loops (noinline), the voice
                                # bounded playback ASRC/mixer, called by the audio ISR
         "fm1_alnk0_irq"]
TOL = 0.10                      # exact (no noise): small edits pass, a grown render loop does not
DIV_W = 8                       # a divide weighs 1 + 8 instructions
NEST = 4                        # an instruction in a loop inside a loop weighs 4, two deep 16, ...
MAXD = 4

dis = sys.argv[1] if len(sys.argv) > 1 else "build/felucca.dis"
budget = sys.argv[2] if len(sys.argv) > 2 else "tests/target_budget.txt"
LABEL = re.compile(r"^([A-Za-z_][A-Za-z_0-9.]*):$")
INSN = re.compile(r"^\s*([0-9a-f]+):\s+((?:[0-9a-f]{2} )+)\s*(.*)$")
TARGET = re.compile(r"goto -?\d+ <[^>]*: ([0-9a-f]+) >")


def functions(path):
    """{name: [(addr, text)]}: a function runs to the next label not starting with '.' (the compiler's
    jump-table labels are inside functions); jump-table data ('< n : 0x.. >') is left out"""
    out, cur, predicated = {}, None, 0
    with open(path) as f:
        for line in f:
            m = LABEL.match(line.strip())
            if m:
                name = m.group(1)
                if not name.startswith("."):
                    cur = out.setdefault(name, []) if name in FUNCS else None
                    predicated = 0
                continue
            m = INSN.match(line)
            if cur is not None and m and not m.group(3).lstrip().startswith("<"):
                text = m.group(3).strip()
                # pi32v2 emits predicated instruction blocks as `if (...) {`
                # followed by ordinary-looking instructions. Their return or
                # goto must retain the false-condition fallthrough in the CFG.
                if predicated and (text.startswith("goto ") or is_return(text)):
                    text = "if (predicated) " + text
                cur.append((int(m.group(1), 16), text))
                if text.startswith(("if ", "ifs ")) and text.endswith("{"):
                    predicated += 1
            elif cur is not None and line.strip().startswith("}"):
                if "else {" not in line:
                    predicated = max(0, predicated - 1)
    return out


def is_return(text):
    return text == "rts" or text.startswith("{pc,") or text.startswith("{pc}")


def cyclic_heads(insns):
    """Reject backward cold-block rejoins that are not cycles.

    A backwards layout is not enough: execution must reach its source again
    from its destination. Keep the historical span weighting after this test.
    Opaque computed branches conservatively reach every node rather than hide
    a genuine loop. Calls return to the following instruction.
    """
    addresses = {a: i for i, (a, _) in enumerate(insns)}
    successors = []
    for i, (_, text) in enumerate(insns):
        edge = TARGET.search(text)
        target = addresses.get(int(edge.group(1), 16)) if edge else None
        following = [i + 1] if i + 1 < len(insns) else []
        if is_return(text):
            successors.append([])
        elif edge:
            successors.append(([target] if target is not None else []) +
                              ([] if text.startswith("goto ") else following))
        elif "goto " in text or "pc = " in text:
            successors.append(range(len(insns)))
        else:
            successors.append(following)

    heads = {}
    for i, (address, text) in enumerate(insns):
        edge = TARGET.search(text)
        if not edge:
            continue
        head = int(edge.group(1), 16)
        if head not in addresses or head >= address:
            continue
        pending, seen = [addresses[head]], set()
        while pending:
            node = pending.pop()
            if node == i:
                heads[head] = max(heads.get(head, address), address)
                break
            if node not in seen:
                seen.add(node)
                pending.extend(successors[node])
    return heads


def cost(insns):
    """loops: one span per loop head (the farthest backward branch to it); an instruction inside d
    spans weighs NEST ** (d - 1) (an inner loop runs several times per pass of the outer one)"""
    heads = cyclic_heads(insns)
    n = divs = calls = 0
    w = 0
    for a, t in insns:
        d = min(sum(1 for h, e in heads.items() if h <= a <= e), MAXD)
        if not d:
            continue
        k = NEST ** (d - 1)
        n += 1
        if re.search(r"= r\d+ / r\d+", t):
            divs += 1
            k *= 1 + DIV_W
        calls += t.startswith("call")
        w += k
    return {"insns": len(insns), "loop": n, "div": divs, "call": calls, "cost": w}


def main():
    if not os.path.exists(dis):
        print(f"target: skip ({dis} missing: run ./build.sh)")
        return 0
    fns = functions(dis)
    res = {n: cost(fns[n]) for n in FUNCS if fns.get(n)}
    missing = [n for n in FUNCS if n not in res]
    base = {}
    if os.path.exists(budget):
        for line in open(budget):
            p = line.split()
            if len(p) >= 2 and not line.startswith("#"):
                base[p[0]] = int(p[1])
    if os.environ.get("BUDGET_UPDATE"):
        with open(budget, "w") as f:
            f.write("# FELUCCA target cost budget (tests/target_budget.py): instructions in the loops of each\n"
                    f"# function in build/felucca.dis, x{NEST} per nesting level, divides x{1 + DIV_W}. The check allows "
                    f"+{TOL * 100:.0f} %.\n# Rewritten by BUDGET_UPDATE=1.\n")
            for n, r in res.items():
                f.write(f"{n} {r['cost']}\n")
        print(f"target: budget {budget} rewritten ({len(res)} functions)")
    fail = 0
    for n in missing:
        print(f"target: FAIL {n} not found in {dis} (renamed? inlined? update FUNCS)")
        fail += 1
    for n, r in res.items():
        b = base.get(n)
        state = "no budget (BUDGET_UPDATE=1 adds it)" if b is None else "ok"
        if b is not None and not os.environ.get("BUDGET_UPDATE"):
            if r["cost"] > b * (1 + TOL):
                state = f"OVER BUDGET (+{(r['cost'] / b - 1) * 100:.0f} %, limit +{TOL * 100:.0f} %)"
                fail += 1
            elif r["cost"] < b * (1 - TOL):
                state = f"note: {(r['cost'] / b - 1) * 100:.0f} % (BUDGET_UPDATE=1 to keep it)"
        print(f"target: {n:15s} {r['insns']:5d} instructions, {r['loop']:4d} in loops, {r['div']} divides, "
              f"{r['call']:2d} calls there: cost {r['cost']:5d} (budget {b if b is not None else '-'}) {state}")
    return 1 if fail else 0


if __name__ == "__main__":
    sys.exit(main())
