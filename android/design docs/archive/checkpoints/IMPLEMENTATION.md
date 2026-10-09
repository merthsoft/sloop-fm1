# Delivery plan and validation gates

Current firmware: **2.4.6 Merthsoft**. Latest Android APK is built and installed on Pixel 7a. See [STATUS.md](../../STATUS.md) for current delivery and user hardware acceptance.

MIDI/octave checkpoint: Generic MIDI output and protocol-10 FM1 octave following are implemented. Perform exposes octave controls and constrains voice leading to the selected register; generic mode alone shows channel override. Latest Workstation results are 65 checks plus 79 kit mappings. Latched chord pitches remain unchanged on octave selection until the next chord press.

UI refinement checkpoint: Compact shared navigation, highlighted selections, expandable FM6/sequence/library sections, and separate Chop/Send sampling pages with cursor dragging, a four-column chop pad bank, capacity display and transfer-stage progress are integrated. User hardware acceptance is recorded in STATUS.md.

Status: integrated Android checkpoint, October 8, 2026. See [STATUS.md](../../STATUS.md) for current
delivery, [INTEGRATION.md](INTEGRATION.md) for evidence, and [NEXT-WAVE.md](../planning/NEXT-WAVE.md) for active work.

## Delivery status

| Slice | Available now | Remaining work / acceptance |
| --- | --- | --- |
| Shell/link | Five workspaces, SLOOP/generic MIDI output, simulator, protocol-10 octave follow | External controller input, multiport profiles and consolidated diagnostics |
| Capture/chop | WAV/capture/recovery, trim/chops/undo, zoom/pan, exact edges and transient proposals | Pitch/root detection, per-chop gain/tuning, tap-along chop and asset browser |
| Convert/send | ADPCM mapping/fit/preview, durable backups, upload/readback/restore | Combined USR3+4, stopped-state integration, broader imports and time stretching |
| Perform | Eight chord pads/joystick, grid/keys/drums/ribbon, recording, presets, latch/register fixes | Configurable mappings, XY macros, fills/punch FX and external controller input |
| Sound | Manual FM6, diagrams/envelopes/copy/swap/numeric entry, bounded prompts, SysEx and history | Hardware A/B/Keep/Restore, banks, all-engine coverage and local synthesis |
| Patterns | App/native editors, continuous looping, gesture capture and reviewed prompt edits | Touch piano roll, richer native inspector, external sync and timing telemetry |
| Storage/library | Named sessions/presets, archives, staged adoption/recovery and retained assets | Indexing/collection, migrations, background save progress and durable undo |
| Firmware USB | Duplex playback implementation, host tests and target budgets; user hardware acceptance | Negotiated return controls and performance-budget follow-up |
| Scenes/arrange/extensions | App scenes/arrangements/CC automation, session staging and stopped-only FM1 sound transfer | Atomic live hardware sound/pattern/tempo transaction and explicit sample dependencies |
| Prompt composition | Sound recipes and existing-material prompt editing | Offline editable loop generation, then optional local model runtime |

## Historical validation checkpoint — October 7, 2026

Independent scenes checkpoint: 20 executable domain scenarios pass, covering capture/revisions,
arrangements, automation, boundary queues and reconciliation, plus persistence round-trips.
The scene domain/test projects and native Android adapter library build with zero warnings/errors.
This evidence does not certify shared-shell integration, runtime UI layout or physical timing.

SoundDesign: 1,327 assertions; Sequencing: 17 scenarios; SampleEncoding: 102 assertions
and nine Python golden fixtures; Protocol/simulator/sampling: 410 checks; Workstation:
61 integration checks including 31,752 chord voicings and MIDI ownership/cancellation.
Android build succeeds with zero warnings/errors; packages and editing/recording/performance
flows were exercised on Pixel 7a. Performance preview without an FM1 is silent.

The app simulator models parameter replies, not DSP, flash or the new native/sample operations.
Separate Workstation protocol fixtures cover those adapters. Existing native FM6/USB host
checks and target builds provide firmware evidence without certifying physical timing.
Audio ISR static budgets fail on both baseline and playback builds; no budget was relaxed.
No firmware was flashed during app integration. More tests are required when the remaining
features are implemented; the target checklist below is not a list of passed checks.

## Decisions to resolve with hardware

- Target phone/tablet models, USB host power and charging, actual input/output route behavior.
- Actual MIDI timestamp support and touch/control latency under USB audio capture.
- FM1 firmware budgets for staged scenes and all-track notifications.
- Native preview backend need based on measured multi-pad performance.
- Exact combined-slot kit layout and controller note mapping from existing code.
- Optional Pocket Operator/physical HiChord model and required audio/sync adapters.

## Test strategy

Use meaningful shared-core and codec tests once implementations exist: serialization,
conversion vectors, ownership cleanup, scheduling semantics and recovery transitions.
Use simulated MIDI/audio adapters for reproducible malformed/reordered/disconnect events.
Simulation cannot certify USB audio routing, power, or live timing; those need real devices.

Keep a physical validation record with device/OS/firmware versions, connection setup, actual
formats, latency/jitter statistics, recording continuity, and reproduction steps. Run firmware
regression checks for changes that touch sequencer, protocol, storage or audio. Avoid claiming
features tested simply because shared libraries compile.

## Definition of a completed slice

The feature works end-to-end, unavailable states are honest, failure is recoverable, original
music survives, relevant checks pass, and build/run instructions describe current behavior.
Update these design contracts when implementation changes a decision. No planned interface,
firmware command, or capability is presented as implemented until both endpoint and UI work.



