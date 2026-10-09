"""Generate the simulator's default-build metadata from the checked-out firmware.

Run: py -3 android/src/Sloop.Simulator/generate_profile.py [--check]
No external Python packages. Fails on unsupported expressions rather than inventing values.
"""
import hashlib
import json
import re
import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[2]
SRC = ROOT / "firmware" / "src"


def clean(text):
    text = re.sub(r"/\*.*?\*/|//[^\n]*", "", text, flags=re.S)
    return re.sub(r"#if FELUCCA_SLICE\b.*?#endif", "", text, flags=re.S)


core = clean((SRC / "core.h").read_text())
params = clean((SRC / "params.c").read_text())
symbols = {}
for body in re.findall(r"enum\s*\{([^}]+)\}", core):
    index = 0
    for token in body.split(","):
        token = token.strip()
        if not token:
            continue
        if "=" in token:
            name, raw = token.split("=", 1)
            index = int(raw.strip(), 0)
            token = name.strip()
        if not re.fullmatch(r"[A-Z][A-Z0-9_]*", token):
            raise ValueError(f"Unsupported enum token: {token}")
        symbols[token] = index
        index += 1
for name in ("NSTEP", "NTRK"):
    symbols[name] = int(re.search(rf"#define {name}\s+(\d+)", core)[1])


def number(raw):
    raw = raw.strip()
    return symbols[raw] if raw in symbols else int(raw, 0)


names = {
    name: re.findall(r'"([^"\\]*)"', body)
    for name, body in re.findall(r"static const char \*const (N_\w+)\[\]\s*=\s*\{(.*?)\};", params, re.S)
}
descriptors = []
for scope, table in enumerate(("TP", "GP")):
    body = re.search(rf"static const param_desc_t {table}\[.*?\]\s*=\s*\{{(.*?)\n\}};", params, re.S)[1]
    for name, kind, label, args in re.findall(r'\[(\w+)\]\s*=\s*(PD|PE)\("([^"]*)",\s*([^)]*)\)', body):
        fields = [v.strip() for v in args.split(",")]
        if kind == "PD":
            fmt, low, high, fallback = map(number, fields)
            choices = []
        else:
            choices = names[fields[0]]
            fmt, low, high, fallback = symbols["F_ENUM"], 0, len(choices) - 1, number(fields[1])
        descriptors.append(dict(Scope=scope, Id=symbols[name], Symbol=name, Format=fmt,
                                Minimum=low, Maximum=high, Default=fallback,
                                Label=label, Unit="", Choices=choices))

engine_table = clean((SRC / "engines.c").read_text())
engine_body = re.search(r"ENGINES\[NENGINES\]\s*=\s*\{(.*?)\};", engine_table, re.S)[1]
engine_names = {}
for path in sorted(SRC.glob("eng_*.c")):
    for name, label in re.findall(r'static const engine_t (ENG_\w+)\s*=\s*\{\s*"([^"]+)"', path.read_text()):
        engine_names[name] = label
engines = [engine_names[name] for name in re.findall(r"&(ENG_\w+)", engine_body)]
version = re.search(r'#define FELUCCA_VERSION "([^"]+)"', (SRC / "ui.c").read_text())[1]
protocol = int(re.search(r"#define ED_PROTO (\d+)u", (SRC / "editor.c").read_text())[1])
sources = ["core.h", "params.c", "engines.c", "ui.c", "editor.c"]
sources += sorted(path.name for path in SRC.glob("eng_*.c"))
profile = dict(Firmware=version, ProtocolVersion=protocol, TrackCount=symbols["NTRK"],
               StepCount=symbols["NSTEP"], ParameterCount=symbols["P_COUNT"],
               GlobalCount=symbols["G_COUNT"], EngineParameterStart=symbols["P_E0"],
               Engines=engines, Parameters=descriptors,
               SourceHashes={name: hashlib.sha256((SRC / name).read_bytes()).hexdigest() for name in sources})
if len([p for p in descriptors if p["Scope"] == 0]) != symbols["P_E0"]:
    raise ValueError("Not every common track parameter was extracted")
if len([p for p in descriptors if p["Scope"] == 1]) != symbols["G_COUNT"]:
    raise ValueError("Not every global descriptor was extracted")
output = json.dumps(profile, indent=2) + "\n"
destination = HERE / "FirmwareProfile.json"
if "--check" in sys.argv:
    if not destination.exists() or destination.read_text() != output:
        sys.exit("Firmware profile is stale. Run generate_profile.py to regenerate it.")
    print("Firmware profile matches checked-out sources.")
else:
    destination.write_text(output, encoding="utf-8")
    print(f"Generated {len(descriptors)} descriptors, {len(engines)} engines, protocol {protocol}.")
