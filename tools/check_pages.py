#!/usr/bin/env python3
"""Fail deployment if the checked-in installer doesn't match the packaged release."""
import hashlib
import re
from pathlib import Path

def check(root):
    version = re.search(r'#define FELUCCA_VERSION "([0-9.]+)"', (root / 'firmware/src/version.h').read_text()).group(1)
    package = root / 'docs/firmware' / f'merthsloop-{version}.fwsc'
    installer = (root / 'docs/webapp/installer/index.html').read_text(encoding='utf-8')
    if not package.exists():
        raise SystemExit(f'Build and regenerate docs before publishing {version}: missing {package.name}')
    for expected in (f'"version": "{version}"', f'"pkg": "../../firmware/{package.name}"',
                     f'"sha256": "{hashlib.sha256(package.read_bytes()).hexdigest()}"'):
        if expected not in installer:
            raise SystemExit(f'Installer/package mismatch: {expected}')
    if not (root / 'docs/webapp/editor/sloop-preview.wasm').exists():
        raise SystemExit('Browser audio build is missing from the published editor.')
    return f'Pages package verified: {version}, {package.stat().st_size} bytes'

if __name__ == '__main__':
    print(check(Path(__file__).resolve().parents[1]))
