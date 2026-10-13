import hashlib
import json
from pathlib import Path
import sys
import tempfile
import unittest

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'tools'))
from check_pages import check

class PagesDeploymentTests(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.addCleanup(self.temp.cleanup)
        self.root = Path(self.temp.name)
        for folder in ('firmware/src', 'docs/firmware', 'docs/webapp/installer', 'docs/webapp/editor'):
            (self.root / folder).mkdir(parents=True)
        self.header = self.root / 'firmware/src/version.h'
        self.header.write_text('#define FELUCCA_VERSION "2.6.0.14"')
        self.package = self.root / 'docs/firmware/merthsloop-2.6.0.14.fwsc'
        self.package.write_bytes(b'test package')
        self.installer = self.root / 'docs/webapp/installer/index.html'
        self.installer.write_text(json.dumps({'version':'2.6.0.14',
            'pkg':'../../firmware/merthsloop-2.6.0.14.fwsc',
            'sha256':hashlib.sha256(self.package.read_bytes()).hexdigest()}))
        self.preview = self.root / 'docs/webapp/editor/sloop-preview.wasm'
        self.preview.write_bytes(b'preview fixture')

    def test_current_package(self):
        self.assertIn('2.6.0.14', check(self.root))

    def test_unbuilt_version_bump(self):
        self.header.write_text('#define FELUCCA_VERSION "2.6.0.15"')
        with self.assertRaisesRegex(SystemExit, 'missing'): check(self.root)

    def test_changed_package(self):
        self.package.write_bytes(b'changed package')
        with self.assertRaisesRegex(SystemExit, 'sha256'): check(self.root)

    def test_old_installer(self):
        self.installer.write_text(self.installer.read_text().replace('2.6.0.14', '2.6.0.13'))
        with self.assertRaisesRegex(SystemExit, 'version'): check(self.root)

    def test_missing_browser_audio(self):
        self.preview.unlink()
        with self.assertRaisesRegex(SystemExit, 'audio build'): check(self.root)

if __name__ == '__main__': unittest.main()
