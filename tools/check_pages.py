#!/usr/bin/env python3
"""Fail deployment if the checked-in installer doesn't match the packaged release."""
import hashlib
import re
from pathlib import Path
import subprocess
import sys
import tempfile
from web_provenance import validate_preview

def check(root, temp_dir=None):
    root = Path(root).resolve()
    version = re.search(r'#define FELUCCA_VERSION "([0-9.]+)"', (root / 'firmware/src/version.h').read_text()).group(1)
    package = root / 'docs/firmware' / f'merthsloop-{version}.fwsc'
    installer = (root / 'docs/webapp/installer/index.html').read_text(encoding='utf-8')
    if not package.exists():
        raise SystemExit(f'Build and regenerate docs before publishing {version}: missing {package.name}')
    for expected in (f'"version": "{version}"', f'"pkg": "../../firmware/{package.name}"',
                     f'"sha256": "{hashlib.sha256(package.read_bytes()).hexdigest()}"'):
        if expected not in installer:
            raise SystemExit(f'Installer/package mismatch: {expected}')
    preview = root / 'docs/webapp/editor'
    validate_preview(root, preview)
    # Regenerate from the checked-in sources, package and verified preview in a
    # disposable output tree. Never repair/publish docs as part of verification.
    with tempfile.TemporaryDirectory(dir=temp_dir) as temporary:
        expected = Path(temporary) / 'site'
        result = subprocess.run([sys.executable, str(root / 'web/make_site.py'),
                                 str(package), version, str(expected), '--preview-dir', str(preview)],
                                capture_output=True, text=True)
        if result.returncode:
            raise SystemExit('Pages regeneration failed: ' + result.stdout + result.stderr)
        for path in expected.rglob('*'):
            if path.is_file():
                rel = path.relative_to(expected)
                published = root / 'docs' / rel
                if not published.is_file() or path.read_bytes() != published.read_bytes():
                    raise SystemExit(f'Stale or missing generated Pages asset: {rel.as_posix()}')
        expected_fw = {p.name for p in (expected / 'firmware').glob('*.fwsc')}
        if {p.name for p in (root / 'docs/firmware').glob('*.fwsc')} != expected_fw:
            raise SystemExit('Stale firmware package retained in docs/firmware')
        expected_app = {p.relative_to(expected / 'webapp') for p in (expected / 'webapp').rglob('*') if p.is_file()}
        published_app = {p.relative_to(root / 'docs/webapp') for p in (root / 'docs/webapp').rglob('*') if p.is_file()}
        if published_app != expected_app:
            raise SystemExit('Stale generated asset retained in docs/webapp')
    return f'Pages package verified: {version}, {package.stat().st_size} bytes'

if __name__ == '__main__':
    print(check(Path(__file__).resolve().parents[1]))
