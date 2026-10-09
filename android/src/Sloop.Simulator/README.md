# FM1 protocol simulator

Runs in-process on Android, desktop tests, or an Android emulator. Implements the current
app connection slice through real encoded/fragmented SysEx: INFO, common parameter DESC,
GET/SET of inert values, PING, track query/selection, and common TRACK_PARAM. Values clamp to generated firmware ranges.
Each simulator instance starts fresh at descriptor defaults; this is not a booted device preset.

Metadata is generated from the checked-out firmware's core enums, parameter descriptors,
engine table/names, version and protocol version. Source hashes identify the exact snapshot.
The extraction targets the default build (FELUCCA_SLICE=0). Regenerate/check with Python 3:

```powershell
python generate_profile.py
python generate_profile.py --check
```

Explicit limits: no DSP/audio, notes, sequencer execution, flash, backup, sample upload,
engine-specific descriptors or action globals. Unsupported requests get no reply; the app
must not present simulated success for them. Add behavior alongside its feature and tests.
Firmware integration tests remain necessary: this independently implemented model can be wrong.
Existing tests/hostsim.c already renders actual firmware DSP to WAV on a supported host toolchain;
it is a useful later native backend, not currently connected to this simulator.

ReplyDelay, FragmentSize and DropNextReply exercise delayed, fragmented and lost replies.
Disposing the transport simulates unplugging. This transport cannot validate Android USB
discovery, power, routing, device latency or resource budgets.
