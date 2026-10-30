# SPDX-License-Identifier: GPL-3.0-only
"""Check descriptor semantics against output captured before compaction.

The fixture hashes cover original metadata, all formatted values/units and real
ED_DESC response bytes. A deliberate descriptor/preset change needs an explicit
fixture review, rather than regenerating expectations during the test.
"""
from pathlib import Path
import hashlib
import json
import subprocess
import sys


def main(oracle, wire, output):
    root = Path(__file__).resolve().parents[1]
    fixture = json.loads((root / 'tests/fixtures/parameter-descriptors-103.json').read_text())
    output = Path(output)
    output.mkdir(parents=True, exist_ok=True)
    for program, name in ((oracle, 'descriptors.txt'), (wire, 'wire.bin')):
        path = output / name
        subprocess.run([str(Path(program).resolve()), str(path.resolve())], check=True)
        data = path.read_bytes()
        assert len(data) == fixture[name]['bytes'], name
        assert hashlib.sha256(data).hexdigest() == fixture[name]['sha256'], name
    print('Parameter metadata, every displayed value/unit and editor descriptor replies match original .103 bytes')


if __name__ == '__main__':
    main(*sys.argv[1:])
