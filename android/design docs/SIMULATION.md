# Hardware-free development

Current delivery: [STATUS.md](STATUS.md). Build and test evidence: [VERIFICATION.md](VERIFICATION.md).

Status: the app's limited parameter simulator, offline editors/conversion/performance touch
preview and separate Workstation protocol fixtures are implemented; checkpoint 2026-10-07.
These are distinct testing paths. Offline Perform is silent and does not claim to simulate
FM1 audio. Actual firmware DSP emulation and a PC bridge remain future work; user hardware
testing is accepted for now.

## Implemented protocol model

Sloop.Simulator runs in-process behind the same IMidiTransport as a real Android connection.
The app exposes Try simulated FM1 and labels the entire connection as simulated. INFO,
common DESC, inert GET/SET, track query/selection, TRACK_PARAM and PING use real SysEx framing
and fragmentation. The same EditorClient executes the handshake and level editing.
Profiles are regenerated from the default firmware build's enum IDs, ranges, defaults,
choices, engine names, version and protocol. Checked-in source hashes expose drift.

The simulator is an independent behavioral model, not a CPU emulator. It does not execute
firmware DSP, audio, sequencing, flash, sample transfer, engine-specific descriptors, or action
globals. Unsupported operations must not manufacture success. Expand supported behavior and
fixtures with each app feature, and keep the coverage list visible in its README.

## Failure and regression testing

Workstation.Tests supplies a separate FirmwareTransport fixture for raw FM6, sample backup/
upload CRC and complete native pattern/lock readback. These operations are not exposed by
the app's Try simulated FM1 mode. The 53 integration checks also cover performance harmony,
overlapping owners and timed cancellation. Existing Protocol.Tests provides 410 protocol,
simulator and sampling checks. A fixture is not execution of firmware/editor.c.

FragmentSize splits replies into arbitrary pieces. ReplyDelay simulates latency. DropNextReply
can leave a mutation applied with its acknowledgment missing; the client must treat this as
uncertain. Disposal simulates unplugging. Test profile freshness before trusting metadata.
An Android emulator can exercise UI/lifecycle and this model without any instrument.
Passing simulation does not establish USB host support, power, Android routing, physical
latency, firmware interrupt safety or actual storage behavior.

## Actual firmware execution

The repository already contains tests/hostsim.c, which includes real DSP, voice, FX and
sequencer sources and writes WAV output. Reuse this harness on its supported host toolchain
for deterministic audio tests before inventing another synth implementation. A later native
backend would expose controlled MIDI/parameter commands, audio blocks, clock advancement,
and a firmware protocol harness with mocked storage and panel state. Pin source/build identity
and compare the C# protocol model against actual editor.c replies.

This host execution avoids emulating the FM1 CPU/peripherals instruction by instruction;
hardware drivers and interrupt scheduling still require device validation. No new native
audio backend is implemented in the current protocol simulator.

## PC-connected instrument

A future PC bridge could relay app protocol bytes to the real FM1 over PC MIDI ports. Its
network/IPC transport would be explicitly identified as a bridge, with connection epochs and
bounded latency. It would test protocol interaction but bypass the phone's USB/audio path.
Do not depend on arbitrary USB passthrough being available in an Android emulator setup.
Wireless Android debugging is another option for observing a real phone while its USB port
is occupied by the FM1, after pairing and network configuration are available.

