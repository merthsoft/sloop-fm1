#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Compile real SLOOP DSP/clock for browser preview (emcc or PREVIEW_CC).

python3 tools/build_preview.py
Then web/make_site.py includes the generated audio assets in the local site.
No JieLi SDK, Docker, or connected FM-1 is required.
"""
from pathlib import Path
import shutil
import subprocess
import sys
import os
import shlex

ROOT = Path(__file__).resolve().parents[1]
OUT = ROOT / 'build/preview'
GEN = OUT / 'gen'  # preview generation must not overwrite the target's packed boot tables


def main():
    custom = os.environ.get('PREVIEW_CC')
    emcc = shutil.which('emcc')
    compiler = shlex.split(custom) if custom else [emcc] if emcc else ['clang', '--target=wasm32'] if shutil.which('clang') and shutil.which('wasm-ld') else None
    if not compiler:
        sys.exit('Install Emscripten or clang with wasm-ld, or set PREVIEW_CC (for example: zig cc -target wasm32-freestanding).')
    GEN.mkdir(parents=True, exist_ok=True)
    OUT.mkdir(parents=True, exist_ok=True)
    for script, header in [('gen_tables.py', 'felucca_tables.h'), ('gen_samples.py', 'felucca_samples.h'),
                           ('gen_drumkits.py', 'felucca_drumkits.h'), ('gen_fm6_patches.py', 'felucca_fm6.h')]:
        subprocess.run([sys.executable, str(ROOT / 'tools' / script), str(GEN / header)], cwd=ROOT, check=True)
    exports = ['init', 'buffer', 'frames', 'rate', 'track', 'param', 'global', 'select', 'step', 'locks',
               'fm6', 'sample', 'kit', 'hit', 'note', 'play', 'position', 'panic', 'render']
    if custom or not emcc:
        shim = ROOT / 'tools' / 'wasm_shim'
        mem = OUT / 'mem.o'
        subprocess.run([*compiler, '-O2', '-fno-builtin', '-ffreestanding', '-c', str(shim / 'mem.c'), '-o', str(mem)], cwd=ROOT, check=True)
        options = ['-ffreestanding', '-nostdlib', '-I' + str(shim), str(mem), '-Wl,--no-entry', '-Wl,--export-memory',
                   '-Wl,-z,stack-size=131072',
                   *['-Wl,--export=preview_' + n for n in exports]]
    else:
        options = ['--no-entry', '-sSTANDALONE_WASM=1', '-sFILESYSTEM=0', '-sINITIAL_MEMORY=16777216',
                   '-sSTACK_SIZE=131072', '-sEXPORTED_FUNCTIONS=' + str(['_preview_' + n for n in exports])]
    subprocess.run([*compiler, 'firmware/host/preview.c', '-O2', '-fwrapv', '-fno-strict-aliasing',
                    '-Wno-unused-function', '-Wno-int-to-pointer-cast', '-I' + str(GEN), '-Ifirmware/src',
                    *options, '-o', 'build/preview/sloop-preview.wasm'], cwd=ROOT, check=True)
    if custom or not emcc:
        mem.unlink()
    for name in ['preview.js', 'preview-worklet.js']:
        shutil.copy(ROOT / 'web' / name, OUT / name)
    print('Browser audio built:', OUT / 'sloop-preview.wasm')


if __name__ == '__main__':
    main()
