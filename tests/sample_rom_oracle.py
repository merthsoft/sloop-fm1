"""Extract the generator's raw branch as an independent decoder fixture."""
from pathlib import Path
import re

text = Path('build/gen/felucca_samples.h').read_text()
start = text.index('{', text.index('SMP_DATA['))
end = text.index('};', start)
raw = bytes(int(n, 0) for n in re.findall(r'0x[0-9a-fA-F]+|\d+', text[start+1:end]))
assert len(raw) == int(re.search(r'#define SMP_HUFF_RAW_N (\d+)u', text)[1])
Path('build/sample-rom-oracle.bin').write_bytes(raw)
