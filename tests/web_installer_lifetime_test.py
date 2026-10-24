"""CLI updater policies at discovery, stage transition, and owned-link cleanup."""
from pathlib import Path
import sys
import unittest
from unittest.mock import patch
from types import SimpleNamespace

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'tools'))
import fm1_install as I


def device(text):
    model, version = text.rsplit('_', 1)
    return SimpleNamespace(id=I.Identity(text, model, int(version)), name='FM-1',
                           link=SimpleNamespace(closed=False, send=lambda _: True))


class LifetimeTests(unittest.TestCase):
    def closeable(self, dev):
        dev.link.close = lambda: setattr(dev.link, 'closed', True)
        return dev

    def test_new_foreign_loader_never_written(self):
        running = self.closeable(device('FM-1_900'))
        foreign = self.closeable(device('ota-FM-1_015'))
        up = I.Updater(None)
        up.check = lambda *args: foreign
        up.write = lambda *args: self.fail('foreign loader written')
        with self.assertRaises(I.InstallError) as error:
            up.install(running, b'image', 'FM-1_900')
        self.assertEqual(error.exception.code, 'foreign')
        self.assertTrue(foreign.link.closed)

    def test_expected_loader_still_installs(self):
        running = self.closeable(device('FM-1_927'))
        own = self.closeable(device('ota-FM-1_900'))
        written = []
        up = I.Updater(None)
        up.check = lambda *args: own
        up.write = lambda dev, *args: written.append(dev)
        up.verify = lambda product, step: product
        self.assertEqual(up.install(running,b'image','FM-1_927'),'FM-1_927')
        self.assertEqual(written,[own])

    def test_errors_close_both_stages(self):
        for stage in ['check','write']:
            with self.subTest(stage=stage), patch.object(I.time, 'sleep'):
                dev = self.closeable(device('ota-FM-1_900' if stage == 'write' else 'FM-1_900'))
                up = I.Updater(None)
                def fail(*args): raise I.InstallError('badreq','out of image')
                up.serve = fail
                with self.assertRaises(I.InstallError):
                    getattr(up,stage)(dev,b'image',lambda *args: None)
                self.assertTrue(dev.link.closed)

    def test_callback_error_closes_before_send(self):
        dev = self.closeable(device('ota-FM-1_900'))
        def fail(*args): raise RuntimeError('callback')
        with self.assertRaises(RuntimeError): I.Updater(None).write(dev,b'image',fail)
        self.assertTrue(dev.link.closed)

    def test_discovery_rejects_two_modes_and_closes(self):
        links = [self.closeable(device(text)).link for text in ['FM-1_900','ota-FM-1_900']]
        backend = SimpleNamespace(input_names=lambda:['Felucca','Felucca Update'],
                                  output_names=lambda:['Felucca','Felucca Update'],
                                  open=lambda name, out:links[0 if name == 'Felucca' else 1])
        with patch.object(I, 'handshake', side_effect=[device('FM-1_900').id,device('ota-FM-1_900').id]):
            with self.assertRaises(I.InstallError) as error:
                I.Updater(backend).find(lambda identity:not identity.loader)
        self.assertEqual(error.exception.code,'ambiguous')
        self.assertTrue(all(link.closed for link in links))


if __name__ == '__main__': unittest.main()
