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
GEN = ROOT / 'build/gen'
OUT = ROOT / 'build/preview'


def main():
    custom = os.environ.get('PREVIEW_CC')
    compiler = shlex.split(custom) if custom else ['emcc'] if shutil.which('emcc') else None
    if not compiler:
        sys.exit('Install Emscripten or set PREVIEW_CC to a freestanding WASM compiler (for example: zig cc -target wasm32-freestanding).')
    GEN.mkdir(parents=True, exist_ok=True)
    OUT.mkdir(parents=True, exist_ok=True)
    for script, header in [('gen_tables.py', 'felucca_tables.h'), ('gen_samples.py', 'felucca_samples.h'),
                           ('gen_drumkits.py', 'felucca_drumkits.h'), ('gen_fm6_patches.py', 'felucca_fm6.h')]:
        subprocess.run([sys.executable, str(ROOT / 'tools' / script), str(GEN / header)], cwd=ROOT, check=True)
    exports = ['init', 'buffer', 'frames', 'rate', 'track', 'param', 'global', 'select', 'step', 'locks',
               'fm6', 'sample', 'kit', 'hit', 'note', 'play', 'position', 'panic', 'render']
    options = ['-nostdlib', '-Wl,--no-entry', '-Wl,--export-memory',
               *['-Wl,--export=preview_' + n for n in exports]] if custom else [
                    '--no-entry', '-sSTANDALONE_WASM=1', '-sFILESYSTEM=0', '-sINITIAL_MEMORY=16777216',
                    '-sSTACK_SIZE=131072', '-sEXPORTED_FUNCTIONS=' + str(['_preview_' + n for n in exports]),
                    ]
    subprocess.run([*compiler, 'firmware/host/preview.c', '-O2', '-fwrapv', '-fno-strict-aliasing',
                    '-Wno-unused-function', '-Wno-int-to-pointer-cast', '-Ibuild/gen', '-Ifirmware/src',
                    *options, '-o', 'build/preview/sloop-preview.wasm'], cwd=ROOT, check=True)
    for name in ['preview.js', 'preview-worklet.js']:
        shutil.copy(ROOT / 'web' / name, OUT / name)
    print('Browser audio built:', OUT / 'sloop-preview.wasm')


if __name__ == '__main__':
    main()
