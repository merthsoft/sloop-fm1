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
import argparse
from web_provenance import preview_inputs, write_preview_manifest

ROOT = Path(__file__).resolve().parents[1]
OUT = ROOT / 'build/preview'
GEN = OUT / 'gen'  # preview generation must not overwrite the target's packed boot tables


def main(out=None):
    global OUT, GEN
    OUT = Path(out).resolve() if out else OUT
    GEN = OUT / 'gen'
    inputs = preview_inputs(ROOT)
    custom = os.environ.get('PREVIEW_CC')
    emcc = shutil.which('emcc')
    compiler = shlex.split(custom) if custom else [emcc] if emcc else ['clang', '--target=wasm32'] if shutil.which('clang') and shutil.which('wasm-ld') else None
    if not compiler:
        sys.exit('Install Emscripten or clang with wasm-ld, or set PREVIEW_CC (for example: zig cc -target wasm32-freestanding).')
    GEN.mkdir(parents=True, exist_ok=True)
    OUT.mkdir(parents=True, exist_ok=True)
    for script, header in [('gen_tables.py', 'felucca_tables.h'), ('gen_samples.py', 'felucca_samples.h'),
                           ('gen_drumkits.py', 'felucca_drumkits.h'), ('gen_fm6_patches.py', 'felucca_fm6.h')]:
        if script == 'gen_samples.py':
            # The sample generator otherwise owns shared build/genwav and its
            # cache. Preview outputs, including these intermediates, are private.
            code = ('import sys; from pathlib import Path; sys.path.insert(0,sys.argv[1]); '
                    'import gen_samples as g; g.GENDIR=Path(sys.argv[2]); '
                    'g.CACHE=Path(sys.argv[3]); g.main(sys.argv[4])')
            subprocess.run([sys.executable, '-c', code, str(ROOT / 'tools'),
                            str(OUT / 'genwav'), str(OUT / 'samples.cache'), str(GEN / header)], cwd=ROOT, check=True)
        else:
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
    flags = ['-O2', '-fwrapv', '-fno-strict-aliasing',
                    '-Wno-unused-function', '-Wno-int-to-pointer-cast', '-I' + str(GEN), '-Ifirmware/src',
                    *options]
    subprocess.run([*compiler, 'firmware/host/preview.c', *flags, '-o', str(OUT / 'sloop-preview.wasm')], cwd=ROOT, check=True)
    if custom or not emcc:
        mem.unlink()
    for name in ['preview.js', 'preview-worklet.js']:
        shutil.copy(ROOT / 'web' / name, OUT / name)
    version = subprocess.check_output([*compiler, '--version'], cwd=ROOT, text=True)
    if inputs != preview_inputs(ROOT):
        raise SystemExit('Preview inputs changed during compilation; rebuild from a stable source tree.')
    write_preview_manifest(ROOT, OUT, {'command': compiler, 'version': version,
                                     'python': sys.version, 'slice': os.environ.get('FELUCCA_SLICE', '0')}, flags)
    print('Browser audio built:', OUT / 'sloop-preview.wasm')


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--out', type=Path, help='Private preview output directory (default: build/preview)')
    main(parser.parse_args().out)
