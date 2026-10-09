# Hardware scene transaction handoff — October 8, 2026

Implemented a complete bounded protocol/staging slice. Live hardware scene application
is **not integrated**: the necessary audio/sequencer/revision hooks are parent-owned.
Current firmware still reports INFO protocol 10; existing Android scene UI keeps the
stopped-only FM6 sound batch with its baseline/readback/Partial/Unknown recovery.

## Changed files (this slice)

- firmware/src/usb.c: command 72 interception in main-loop OTA frame retrieval;
  USB reset/suspend/frozen frame/deconfiguration/detach invalidate staging.
  Existing pre-task USB playback edits were preserved.
- firmware/src/scene_transaction.c and .h: one RAM buffer, structural validation,
  full CRC, monotonically increasing token, epoch, bounded chunk retries,
  prepare/commit/cancel/status, exact-boundary lifecycle, guarded descriptors.
- android/src/Sloop.Protocol/SceneTransaction.cs: schema builder/validator, explicit
  flash dependency states, wire client, negotiated limits and status distinction.
- android/src/Sloop.Protocol/SCENE-TRANSACTION.md: exact operation/reply/schema contract.
- android/src/Sloop.SceneTransaction.Tests/{Sloop.SceneTransaction.Tests.csproj,Program.cs}.
- tests/{scene_transaction_test.c,scene_transaction_usb_test.c,run_scene_transaction.sh}.
- Component design updates: SCENES.md, FIRMWARE.md, and this handoff.

No shared shell/session/connection/solution/central runner changes. No firmware
editor/ui/version/seq/audio/core changes. SceneHardwareEditor.cs and
SceneHardwareBatch.cs required no edits: retaining fallback is intentional.

## Exact parent integration hooks

1. Register `sc_hooks` once while stopped before USB servicing. All six members
   (`revision`, `validate`, `apply`, `enter`, `leave`, `schedule`) are mandatory;
   flags remain zero and Begin Unsupported until all are installed.
   Functions and state are static in the existing single compilation unit.
   Add forward declarations before usb.c or a dedicated declaration header, and
   define callbacks after engine/sequencer definitions. Do not add another global buffer.
2. `revision()` returns a nonwrapping u28 revision for the complete destination
   sound/pattern/tempo/dependency state. Every relevant local/remote mutation increments
   it, including editor.c writes, panel edits, FM6 bank/preset changes, sample flash
   writes, pattern recording, project adoption and engine switches. Guard against
   external edits from Prepare through Apply. If revision reaches u28 exhaustion,
   disable capability until a fresh connection epoch/revision baseline is established.
   Staging does not manufacture a revision from a stale host snapshot.
3. `validate(payload,length)` returns a SC_* code. Enforce FM6 destinations and
   packed patch field ranges, seven macro ranges/routing, lockable param IDs/ranges,
   duplicate lock rejection, track length/division compatibility and actual sample
   slot dependencies. Resolve native scene references to immutable bytes before
   host Build; app MIDI patterns are not interchangeable with this native schema.
   Flash upload is a separate safe stopped operation. There is no drum replacement.
4. `enter()/leave()` provide IRQ-safe exclusion and compiler ordering for compact
   descriptor operations. Main-loop CRC/schema validation is outside the guard.
   Hook callbacks must not yield/send USB or block. Stage cannot coexist with a
   queued apply; queued payload is immutable until audio completes or main cancels.
5. `schedule(absoluteTick,kind)` validates an exact, strictly future beat/bar/phrase
   boundary in the current transport epoch and returns SC_OK or an explicit error.
   It must guarantee callback insertion before sequencer events, using the same
   absolute u28 tick domain as Commit (no tick wrap during a queued transaction).
   Call `sc_tx_boundary(tick, sc_tx.boundary_kind)` exactly when that requested kind
   is actually due, before outgoing notes/locks/new events. Do not call beat then bar
   callbacks with differing kinds at the same requested tick. A skipped/late tick
   fails closed; calling only when current tick equals due would leave a missed
   transaction queued forever, so detect crossing and invoke the failure path.
6. `apply(payload,length)` is an infallible bounded engine operation after validation:
   release outgoing sequencer-owned notes, restore macros/FM6 patch, all 64 synth
   steps/micro/fill/locks, lengths/divisions and tempo together; clear outgoing locks,
   reset native scheduling origins consistently, and publish a fresh revision.
   Existing FM6 patch setter/tempo paths require an audio-safety/deadline audit.
   No flash/USB/allocation or fallible operations in this callback. A revision
   conflict at due time skips apply entirely. Applied status is recorded only after
   callback completion; repeating Commit never repeats application.
7. Stop/seek/restart/project adoption and preparation inactivity expiry call
   `sc_tx_reset()` in an appropriate guarded main-loop lifecycle path. USB loss
   already latches immediate cancellation before its deferred reset. Choose and
   implement a bounded inactivity timeout; it is intentionally not tied to ISR
   wall-clock guesses here. No parent lifecycle integration is currently present.
8. After integration checks, advertise ED_PROTO=11 in editor.c and add the extension
   link/command to web/EDITOR_PROTOCOL.md without changing existing command layouts.
   No firmware version bump/build/install/flash was performed by this chat.
9. Android parent may construct `SceneTransactionClient` from its existing
   EditorClient, gated by INFO >=11, call CapabilitiesAsync, then StageAsync with
   the negotiated epoch, unique monotonic token and observed revision. CommitAsync
   queues only; poll StatusAsync and publish success exclusively for Applied with
   matching epoch/token/boundary and observed revision. Unsupported routes to the
   stopped-only sound batch; do not silently use fallback while playing.
   Native pattern serialization/session/transport/UI integration remains parent work.
   Timeout/disconnect/cancellation faults EditorClient: reconnect/read complete actual
   state before further writes. If epoch changed, old status cannot prove the result;
   reconcile observed state, retaining existing Partial/Unknown recovery.

## Budget and validation evidence

Read-only ELF section audit of existing build/felucca.elf (no rebuild): text 566,176,
RAM text 2,964, data 2,200, BSS 90,740, pool 334,560. Data+BSS is 92,940/98,304,
leaving 5,364. Pool has 9,504 remaining of 344,064. The older 90,612 checkpoint is
not the current ELF. No pool allocation or flash-layout change is introduced.

Host tests measure `sizeof(sc_tx_state)=3104`, hooks=48 on 64-bit Linux;
32-bit target hooks predict 24, plus loss latch=1: 3,129 new static bytes before
linker alignment, leaving about 2,235 for integration. Target compiler may discard
disabled staging while hooks are unregistered; final enabled target build must measure
actual RAM/image, stack headroom and bounded callback timing. No timing claim is made.

- `dotnet run --project android/src/Sloop.SceneTransaction.Tests`: **25 checks pass**,
  including schema/dependency rejection, pack7 roundtrip/full CRC, negotiation,
  complete serialized Stage/Commit/Status/Cancel and faulted timeout epoch.
  Payload=2,956 raw bytes; maximum wire frame=125 (existing RX buffer=640).
  31 Data frames + Begin + Prepare = 33 stage requests, plus capability request.
- `dotnet run --project android/src/Sloop.SceneHardware.Tests`: **8 fault checks pass**;
  original fallback and reconciliation unchanged.
- `tests/run_scene_transaction.sh` with Zig 0.13.0 C compiler and undefined-behavior
  checks: lifecycle, idempotence, out-of-order/duplicate/conflicting chunks, CRC,
  incomplete preparation, dependency/engine-validator errors, revision conflicts,
  schedule rejection, cancellation, late ticks, applied-once, loss and epoch reset;
  **20,000 malformed packet cases pass**. Real OTA-enabled USB interception, legacy
  INFO passthrough and loss/detach reset also pass.
- Existing `tests/run_usb_playback.sh`, OUT=build/scene-transaction-usb-regression:
  all CDC variants and loader pass. Those playback fixtures disable OTA; dedicated
  scene_transaction_usb_test separately compiles and exercises the enabled OTA path.

Ubuntu has no system cc. Existing compiler directories were incomplete; the existing
build/host-tools/zig.tar.xz was extracted into /tmp/sloop-scene-transaction-tools for
host tests. Reproduce via WSL Ubuntu from /mnt/c/code/sloop-fm1:
`CC='/tmp/sloop-scene-transaction-tools/zig-linux-x86_64-0.13.0/zig cc' sh tests/run_scene_transaction.sh`.
An earlier extraction into ignored build/scene-transaction-tools also completed,
with its lifecycle tests passing; it contains only compiler/test artifacts.

## Integration audit — October 8, 2026

The shared host regression runner now includes the dedicated transaction and
OTA-enabled USB tests. These cover both production's compile-disabled extension
(unknown command passthrough, no staging allocation) and a flagged staging build
(zero capability flags and Unsupported Begin without reserving staging state). Existing
USB playback variants are rerun for transport compatibility. Firmware 2.4.7 retains
INFO protocol 10, `FELUCCA_SCENE_TX=0`, and the stopped-only sound batch; the reserved extension is documented
in web/EDITOR_PROTOCOL.md.

Engine hook activation is intentionally deferred after source audit. `song.tick`
increments once per playing audio block; musical phase uses `clk_pos` in sample × BPM
units and `clk_beat`, including external MIDI clock advances. Treating block counts
as beat/bar boundaries would miss or misplace transitions. A complete scene revision
must also cover panel, editor, recording, preset/project and sample mutations before
Prepare/Apply conflicts can be guaranteed. FM6 patch installation and native sequencer
reset need a bounded boundary callback audit. Registering partial hooks would create
an unsafe atomicity claim, so this integration does not do so.

Fresh production 2.4.7 target build passes: image 571,340 bytes, static RAM
92,948/98,304 bytes (5,356 free), pool 334,560/344,064, RAM code 925 instructions
with no calls, and HAL-only register access. The enabled staging experiment required
96,052 bytes static RAM; default-off saves its 3,104-byte buffer. Package
build/sloop-2.4.7-Merthsoft.fwsc is 610,066 bytes, identity FM-1_900.
Host scene tests, all USB playback/loader variants, scale/chord/latch tests,
project migration/roundtrip, sequencing, and the 20,000-frame UI fuzz pass.
No flash or install server was performed by this integration agent.

## Remaining limitations

No installed engine hooks, complete device revision tracking, musical scheduling,
inactivity expiry or atomic hardware application yet. No hardware-test gate added:
user acceptance is preserved. Target budget/deadline validation must accompany the
parent's actual boundary integration. No claim of atomic sample upload, whole-device
scene switching, native reference resolution, durable transaction status across reboot,
multiple queued scenes or transparent retry after an ambiguous host timeout.
