import hashlib
import json
from pathlib import Path
import sys
import tempfile
import unittest
import shutil
import subprocess

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'tools'))
from check_pages import check
from web_provenance import write_preview_manifest, preview_inputs, input_digest

ROOT = Path(__file__).resolve().parents[1]

class PagesDeploymentTests(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.addCleanup(self.temp.cleanup)
        self.root = Path(self.temp.name)
        for folder in ('firmware/src', 'tools', 'build/preview/gen'):
            (self.root / folder).mkdir(parents=True)
        shutil.copytree(ROOT / 'web', self.root / 'web', ignore=shutil.ignore_patterns('__pycache__'))
        shutil.copyfile(ROOT / 'tools/web_provenance.py', self.root / 'tools/web_provenance.py')
        self.header = self.root / 'firmware/src/version.h'
        self.header.write_text('#define FELUCCA_VERSION "2.6.0.14"')
        (self.root / 'firmware/src/engine.c').write_text('/* DSP fixture input */')
        raw = bytearray(4096)
        for i in range(20):
            raw[i*48+47] = (ord('FM-1_900'[i])+i+1) if i < 8 else 0x7d
        raw[2000:2016] = b'FELUCCA-LOADER-1'
        pkg = self.root / 'input.fwsc'
        pkg.write_bytes(raw)
        build = self.root / 'build/preview'
        (build / 'sloop-preview.wasm').write_bytes(b'\0asm\x01\0\0\0')
        (build / 'gen/fixture.h').write_text('/* generated fixture */')
        write_preview_manifest(self.root, build, {'version':'fixture'}, ['-O2'])
        subprocess.run([sys.executable, str(self.root / 'web/make_site.py'), str(pkg),
                        '2.6.0.14', str(self.root / 'docs')], check=True, capture_output=True)
        self.package = self.root / 'docs/firmware/merthsloop-2.6.0.14.fwsc'
        self.installer = self.root / 'docs/webapp/installer/index.html'
        self.preview = self.root / 'docs/webapp/editor/sloop-preview.wasm'

    def test_current_package(self):
        self.assertIn('2.6.0.14', check(self.root))

    def test_unbuilt_version_bump(self):
        self.header.write_text('#define FELUCCA_VERSION "2.6.0.15"')
        with self.assertRaisesRegex(SystemExit, 'missing'): check(self.root)

    def test_changed_package(self):
        self.package.write_bytes(b'changed package')
        with self.assertRaisesRegex(SystemExit, 'sha256'): check(self.root)

    def test_old_installer(self):
        self.installer.write_text(self.installer.read_text(encoding='utf-8').replace('2.6.0.14', '2.6.0.13'), encoding='utf-8')
        with self.assertRaisesRegex(SystemExit, 'version'): check(self.root)

    def test_missing_browser_audio(self):
        self.preview.unlink()
        with self.assertRaisesRegex(SystemExit, 'audio build'): check(self.root)

    def test_changed_editor_source(self):
        path = self.root / 'web/editor.html'
        path.write_bytes(path.read_bytes() + b'\n<!-- source fix -->')
        with self.assertRaisesRegex(SystemExit, 'Stale or missing'): check(self.root)

    def test_missing_worklet(self):
        (self.root / 'docs/webapp/editor/preview-worklet.js').unlink()
        with self.assertRaisesRegex(SystemExit, 'Stale or missing'): check(self.root)

    def test_source_line_endings_do_not_change_identity(self):
        path = self.root / 'firmware/src/engine.c'
        path.write_bytes(b'/* DSP */\nint value = 1;\n')
        lf = preview_inputs(self.root)
        path.write_bytes(path.read_bytes().replace(b'\n', b'\r\n'))
        self.assertEqual(lf, preview_inputs(self.root))
        path.write_bytes(b'/* DSP */\r\nint value = 2;\r\n')
        self.assertNotEqual(lf, preview_inputs(self.root))

    def test_site_regeneration_ignores_source_checkout_line_endings(self):
        # Linux CI must reproduce a release packaged from a Windows checkout.
        for path in (self.root / 'web').rglob('*'):
            if path.suffix in {'.html', '.js', '.txt'}:
                data = path.read_bytes().replace(b'\r\n', b'\n')
                path.write_bytes(data.replace(b'\n', b'\r\n'))
        self.assertIn('2.6.0.14', check(self.root))

    def test_binary_line_endings_remain_significant(self):
        path = self.root / 'sample.wav'
        path.write_bytes(b'RIFF\r\nDATA')
        before = input_digest(path)
        path.write_bytes(b'RIFF\nDATA')
        self.assertNotEqual(before, input_digest(path))

    def test_changed_dsp_input(self):
        (self.root / 'firmware/src/engine.c').write_text('/* changed DSP */')
        with self.assertRaisesRegex(SystemExit, 'inputs are stale'): check(self.root)

    def test_invalid_wasm(self):
        self.preview.write_bytes(b'not even wasm')
        with self.assertRaisesRegex(SystemExit, 'WebAssembly'): check(self.root)

    def test_changed_wasm(self):
        self.preview.write_bytes(self.preview.read_bytes() + b'other binary')
        with self.assertRaisesRegex(SystemExit, 'does not match'): check(self.root)

    def test_missing_provenance(self):
        (self.preview.parent / 'sloop-preview.inputs.json').unlink()
        with self.assertRaisesRegex(SystemExit, 'manifest'): check(self.root)

    def test_retained_package(self):
        (self.package.parent / 'merthsloop-old.fwsc').write_bytes(b'old')
        with self.assertRaisesRegex(SystemExit, 'retained'): check(self.root)

    def test_retained_web_asset(self):
        (self.preview.parent / 'old-audio.wasm').write_bytes(b'old')
        with self.assertRaisesRegex(SystemExit, 'retained'): check(self.root)

if __name__ == '__main__': unittest.main()
