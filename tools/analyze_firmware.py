#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Firmware compiler diagnostics and Clang core/dead-store analysis.

Run in the Linux target-toolchain environment after generating build/gen:
  JIELI_TOOLCHAIN=/path/to/toolchain python3 tools/analyze_firmware.py
Unused functions are reported, not automatically deleted: tests, optional
builds, hardware callbacks and the linker map determine whether code is dead.
"""
import argparse
import json
import os
from pathlib import Path
import re
import subprocess

ROOT = Path(__file__).resolve().parents[1]


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("--output", type=Path, default=ROOT / "build/static-audit/current")
    args = ap.parse_args()
    tc = os.environ.get("JIELI_TOOLCHAIN")
    cc = Path(tc or "") / "pi32v2/bin/clang"
    if not tc or not cc.is_file():
        ap.error("JIELI_TOOLCHAIN must name the Linux JieLi target toolchain")
    if not (ROOT / "build/gen/felucca_tables.h").is_file():
        ap.error("generate/build the firmware first; build/gen is read without modification")
    out = args.output.resolve()
    out.mkdir(parents=True, exist_ok=True)
    common = [str(cc.resolve()), "-target", "pi32v2", "-DFELUCCA_BOOT_TABLES=1",
              "-Ifirmware/hal", "-Ifirmware/src", "-Ibuild/gen"]
    analyzer = ["--analyze", "-Xanalyzer", "-analyzer-checker=core,deadcode.DeadStores",
                "-Xanalyzer", "-analyzer-output=text"]

    def run(name, flags, source="firmware/src/felucca.c"):
        cmd = common + flags + [source]
        result = subprocess.run(cmd, cwd=ROOT, text=True, stdout=subprocess.PIPE,
                                stderr=subprocess.STDOUT)
        (out / (name + ".log")).write_text(result.stdout)
        return {"exit_code": result.returncode, "diagnostics": result.stdout,
                "command": cmd}

    probe = out / "analyzer-probe.c"
    probe.write_text("int main(void){int *p=0;return *p;}\n")
    control = run("probe", analyzer, str(probe))
    if control["exit_code"] or "Dereference of null pointer" not in control["diagnostics"]:
        raise SystemExit("Analyzer control failed; do not interpret silent results as clean")
    result = {"probe_detected_null": True, "warnings": run("warnings", [
        "-Oz", "-fno-builtin", "-Wall", "-Wunused-function", "-Wunused-variable",
        "-Wunreachable-code", "-Wunreachable-code-aggressive", "-fsyntax-only"])}
    result["unused_functions"] = re.findall(r"warning: unused function '([^']+)'",
                                             result["warnings"]["diagnostics"])
    result["analyzers"] = {}
    for name, flags in [("production", []), ("cdc", ["-DFELUCCA_CDC=1"]),
                        ("single-core", ["-DFELUCCA_CPU1=0"]),
                        ("ram-only", ["-DFELUCCA_FLASH=0", "-DFELUCCA_OTA=0"])]:
        result["analyzers"][name] = run(name, analyzer + flags)
    (out / "results.json").write_text(json.dumps(result, indent=2) + "\n")
    fail = result["warnings"]["exit_code"] != 0
    print(f"Unused in production: {len(result['unused_functions'])} functions (triage required)")
    for name, r in result["analyzers"].items():
        bad = r["exit_code"] != 0 or bool(re.search(r"warning:|error:", r["diagnostics"]))
        fail |= bad
        print(f"{name}: {'FAIL (see log)' if bad else 'no diagnostics'}")
    print(f"Logs and commands: {out}")
    return int(fail)


if __name__ == "__main__":
    raise SystemExit(main())
