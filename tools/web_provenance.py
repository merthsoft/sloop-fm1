"""Deterministic source and output identities for the browser DSP release."""
import hashlib
import json
from pathlib import Path

PREVIEW_MANIFEST = 'sloop-preview.inputs.json'
TEXT_INPUTS = {'.c', '.h', '.py', '.json', '.txt', '.md', '.csv'}

def input_digest(path):
    # Git checks text out as CRLF or LF depending on the host. Hash the same
    # source identity on either; audio/binary assets remain byte-exact.
    path = Path(path)
    data = path.read_bytes()
    if path.suffix in TEXT_INPUTS:
        data = data.replace(b'\r\n', b'\n')
    return hashlib.sha256(data).hexdigest()

GENERATORS = ('build_preview.py', 'web_provenance.py', 'gen_tables.py',
              'gen_samples.py', 'gen_waves.py', 'sampleio.py',
              'gen_drumkits.py', 'gen_fm6_patches.py', 'drumkit_levels.json')


def digest(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def preview_inputs(root):
    root = Path(root)
    files = [root / 'firmware/host/preview.c']
    files += sorted((root / 'firmware/src').glob('*.[ch]'))
    files += sorted((root / 'tools/wasm_shim').glob('*'))
    files += [root / 'tools' / name for name in GENERATORS]
    files += sorted((root / 'assets/samples-cc0').rglob('*'))
    return {p.relative_to(root).as_posix(): input_digest(p) for p in sorted(set(files)) if p.is_file()}


def write_preview_manifest(root, out, compiler, options):
    out = Path(out)
    manifest = {'format': 2, 'inputs': preview_inputs(root),
                'wasm_sha256': digest(out / 'sloop-preview.wasm'),
                'compiler': compiler, 'options': options,
                'generated': {p.name: digest(p) for p in sorted((out / 'gen').glob('*.h'))}}
    (out / PREVIEW_MANIFEST).write_text(json.dumps(manifest, indent=2, sort_keys=True) + '\n', encoding='utf-8')


def validate_preview(root, directory):
    directory = Path(directory)
    wasm = directory / 'sloop-preview.wasm'
    if not wasm.is_file():
        raise SystemExit('Browser audio build is missing; rebuild preview before packaging.')
    if wasm.read_bytes()[:8] != b'\0asm\x01\0\0\0':
        raise SystemExit('Browser audio build is not a WebAssembly module.')
    try:
        manifest = json.loads((directory / PREVIEW_MANIFEST).read_text(encoding='utf-8'))
    except (OSError, ValueError) as error:
        raise SystemExit('Browser preview input manifest is missing or invalid; rebuild preview.') from error
    if manifest.get('format') != 2 or manifest.get('inputs') != preview_inputs(root):
        raise SystemExit('Browser preview inputs are stale; rebuild preview from current source.')
    if manifest.get('wasm_sha256') != digest(wasm):
        raise SystemExit('Browser preview binary does not match its input manifest; rebuild preview.')
    if not manifest.get('compiler') or not manifest.get('options') or not manifest.get('generated'):
        raise SystemExit('Browser preview build provenance is incomplete; rebuild preview.')
    return manifest
