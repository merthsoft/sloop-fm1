"""Explicitly refresh reviewed fixtures from the firmware's factory generator. Run at repo root.
This never writes to firmware/web; golden data stays checked in for independent C# tests.
"""
import hashlib
import json
import runpy
from pathlib import Path

root = Path(__file__).resolve().parents[3]
source = root / 'tools/gen_fm6_patches.py'
factory = runpy.run_path(str(source))
golden = Path(__file__).parent / 'Golden'
library = root / 'android/src/Sloop.SoundDesign/Factory'
golden.mkdir(exist_ok=True)
library.mkdir(exist_ok=True)
manifest = {'source': 'tools/gen_fm6_patches.py', 'sha256': hashlib.sha256(source.read_bytes()).hexdigest(), 'voices': []}
for i, (label, voice) in enumerate(factory['PATCHES']):
    packed = bytes(factory['pack'](voice))
    (golden / f'{i}.voice.bin').write_bytes(bytes(voice))
    (golden / f'{i}.packed.bin').write_bytes(packed)
    (library / f'{i}.bin').write_bytes(packed)
    manifest['voices'].append({'index': i, 'label': label, 'name': bytes(voice[145:]).decode().rstrip(),
        'packedSha256': hashlib.sha256(packed).hexdigest()})
(golden / 'manifest.json').write_text(json.dumps(manifest, indent=2) + '\n')
