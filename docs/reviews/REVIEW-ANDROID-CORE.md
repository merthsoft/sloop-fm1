# Android core review

## Implementation handoff — APP-1 and APP-2

Implemented on 2026-10-23 after approval of the synthesis plan. The historical review below remains the record of the original findings.

- **APP-1:** `LiveNotes` now commits the complete desired owner/count state before sending, tracks unsent versus ambiguous wire transitions, and retains failed cleanup for later release/panic. An ambiguous transition is reconciled with off before restoring a still-owned pitch at its remembered velocity. Normal revoice still releases outgoing pitches before attacks and leaves common tones untouched. Panic attempts every obligation even if one send fails. An explicit `Forget` drops obligations only when their connection epoch ends; `Fm1Connection.Disconnect` invokes it after best-effort release so a failed old-device off cannot block a new connection or be replayed to it. `PerformancePlayer` exposes the same epoch discard under its existing ownership lock and cancels its jobs before forgetting.
- **APP-2:** Hardware performance initialization is idempotent for a healthy session on the same editor. The adapter owns its renewal task, cancels and joins it on retirement, and clears the captured old session before allowing replacement discovery/publication. Retirements remain awaitable after their fields are detached, preventing a concurrent refresh from racing an older CLEAR. Generation, editor identity and cancellation checks reject stale async initialization. A retiring old session never uses the replacement session fields.
- Added the dedicated automatically discovered runner `android/src/Sloop.CoreOwnership.Tests`. It uses production Workstation classes and links the actual Android `Services/HardwarePerformance.cs`; shell stubs replace only the platform handler, connection snapshot and current-client plumbing. No protocol/session behavior is reimplemented by the shell.

**Failure-before evidence:** the new runner was also copied to the original frozen private source. Its note test fails with the original `LiveNotes.Set` `KeyNotFoundException`; its hardware-only portion fails the repeated-song-refresh invariant (multiple discoveries/session owners). Compile-only private shims supply the newly added epoch-discard API and task accessor, neither reached before those failures. Logs: `build/code-review-android-core-51/baseline-ownership-notes.log` and `baseline-ownership-renewal.log`. The original unmodified-adapter reproduction remains in `repro.log`.

**Passing validation:** current private implementation snapshot under `build/code-review-android-core-51/implementation/src`, with logs named `implementation-*.log`:

| Runner | Result |
| --- | --- |
| Sloop.CoreOwnership.Tests | 42 checks: failure before/after accepting first/middle/final chord on/off; repeated release/panic; shared pitches; failed revoice and latch retrigger; timed-job cleanup; explicit epoch discard; repeated song refresh; close/join; cancelled discovery; reconnect and same-editor reopening during in-flight renewal |
| Sloop.Transport.Tests | 26 checks passed |
| Sloop.Workstation.Tests | 142 integration + 79 kit-mapping checks passed |
| Sloop.ExternalMidiInput.Tests | Decoder, ownership, sustain, channel reset, harmony and timed disconnect checks passed |
| Sloop.HardwarePerformance.Tests | 62 checks passed |
| Sloop.PerformanceControls.Tests | 3,035 checks passed |
| Sloop.Protocol.Tests | 410 protocol/simulator/sampling checks passed |

`git diff --check` passed for the assigned production changes. Initial implementation validation caught off/on ordering drift due to reused dictionary slots; the final implementation explicitly retains release-before-attack ordering and the existing musical-behavior regressions pass. Only assigned production files, this report and the dedicated runner were edited. No commit, version bump, package generation, publication or hardware operation was performed. No parent hook remains outstanding.

**Limits:** no APK/device test was performed; the Android adapter is exercised as linked production C# with a platform shell and deterministic MIDI transport. A synchronous MIDI sender cannot reveal whether a throwing call accepted its bytes; recovery can therefore reattack a pitch that remains logically held, after sending off to reconcile it. This behavior is restricted to ambiguous failures, preserves shared owners, and retains the cleanup obligation if recovery itself fails. Real USB transport ambiguity, musical feel after recovery and hardware lease timing remain device-validation limits. No size, speed or battery improvement is claimed.

Review date: 2026-10-23. Reviewed the current working-tree Android source, copied into `build/code-review-android-core-51/src` for executable checks. No production source, shared build output, Git reference or other report was changed. No device operations were performed. Repository search found no `AGENTS.md`. Read the existing firmware optimization, architectural headroom and redundancy reports; none of their integrated work is counted here.

## Integration recommendation

Fix the two confirmed P2 ownership/lifetime defects below in separate small changes with failure-injection regressions. Neither requires a protocol, firmware, musical feature, factory-content or storage-format change. Existing functional runners passed, but they do not exercise these failures. There is no measured size/speed optimization to integrate from this review.

## Ranked confirmed findings

### 1. P2 — A failed note-off corrupts shared note ownership and removes the panic retry

Location: `android/src/Sloop.Workstation/Performance.cs:86–88` (`LiveNotes.Set`); callers include block/timed `PerformancePlayer` at lines 111, 118, 122 and 126.

Trigger: hold a note, let the sender throw on its note-off, then release that owner again. The method decrements/removes `counts[n]` before sending, but updates `owners` only after every send succeeds. One thrown send therefore leaves the owner referencing a pitch absent from `counts`. The next release throws `KeyNotFoundException` at line 86. `Panic` also cannot retry that pitch because it enumerates `counts.Keys` at line 91. If the receiver remains connected and the failed write did not deliver the off, the sounding note is no longer represented by panic's cleanup set. A note-on failure has the related partial-update problem: it can increment the count without ever publishing the owner.

Validation: executable repro against the unchanged private copy holds pitch 60, injects one `IOException` on off, restores the sender, and observes `KeyNotFoundException` on repeated release. Evidence: `build/code-review-android-core-51/repro/Program.cs`, `repro.log`. This is a deterministic state-invariant failure; it does not depend on an actual Android USB error.

Why current tests miss it: `Sloop.Transport.Tests` and the ownership checks in `Sloop.Workstation.Tests` use successful list-append senders for shared pitches/retriggers/panic. The performance-controls tests inject failure for modulation/macro reset, but do not inject failure into `LiveNotes.Set`.

Minimal fix direction: make ownership updates and the cleanup obligation coherent across sender exceptions. For example, process removals/additions one at a time and publish each successful ownership transition, retaining a failed/ambiguous off in a cleanup set until off succeeds or the connection epoch is explicitly forgotten. Alternatively fault this note-ownership epoch and perform best-effort cleanup of the union of old and new pitches before clearing it. Merely catching `KeyNotFoundException`, or clearing dictionaries before off, hides the inconsistency and loses cleanup. Do not weaken shared-owner counts or issue off for another owner's sustained pitch.

Meaningful regression: use a sender that tracks receiver state and can throw both before and after accepting a chosen on/off. Cover first and middle notes of a chord, repeated release, panic, shared owners, revoice/retrigger and timed-job finalization. Assert no dictionary exceptions, no silently orphaned cleanup obligations, and balanced receiver state once transport recovers or the epoch is explicitly discarded.

### 2. P2 — Song-load refresh replaces the renewal owner without cancelling its timer

Location: `android/src/Sloop.Android/Services/HardwarePerformance.cs:14–16`; close only cancels the currently stored source at lines 42–44. Reachable from `DeviceEditing.cs:75–79` → `Fm1Connection.cs:125`. The song-load caller is `Fm1SongLibraryEditor.cs:38`.

Trigger: connect to protocol >=15 firmware supporting hardware performance, then load songs without disconnecting. Every post-load handshake calls `InitializeHardwarePerformanceAsync`, which overwrites both `hardwarePerformance` and `hardwareRenewal`. The previous periodic task still owns its old session/token and continues waking every 200 ms. The prior song operation normally cleared its gesture owners, so this ordinary path leaks an idle renewal task/timer/source per load rather than necessarily sending extra MIDI. `CloseHardwarePerformanceAsync` only knows the newest source; old loops stop when the outer connection epoch is cancelled, not when replaced or individually closed. Continued use on one connection accumulates work and retained objects.

Validation: the repro compiles the actual copied `HardwarePerformance.cs` with a minimal platform shell, real `EditorClient`/session classes and a deterministic capability transport. Initialize twice, retain the first source, then close: first source has `IsCancellationRequested == false` both after replacement and after close. Evidence: `build/code-review-android-core-51/repro/Program.cs`, `repro.log`. Real Android timer wake/battery cost was not profiled.

Why current tests miss it: hardware-performance runners exercise the protocol/session lease owner, not the Android connection adapter's repeated initialization across song-load handshakes.

Minimal fix direction: explicitly retire the previous renewal owner before publishing a replacement; keep/await its task where practical, and clear the old session before any new gestures can start. Make repeated initialization for the same editor idempotent if capabilities need no refresh. Preserve the `client == editor` and epoch guards around asynchronous discovery/cleanup so a stale refresh cannot replace a new connection's session.

Meaningful regression: initialize, perform the same clear-and-refresh sequence as song load several times, and assert exactly one active renewal loop and one active session; close must stop all its owned loops. Include reconnect/cancel during discovery and while retirement waits on a queued wire request. Avoid a regression that merely checks a private field was reassigned.

## Coverage map and clean observations

The actual project is C#/.NET 10, not Kotlin/Java. `Sloop.Android` targets `net10.0-android`, minimum Android API 26; it references Core, Protocol, Simulator, Workstation and Scenes.Android. Workstation references Core, Protocol, Sequencing, SoundDesign and SampleEncoding. Domain tests are executable console runners. Android binding calls and lifecycle integration cannot be proved by those runners.

| Area | Files inspected | Observations / boundary |
| --- | --- | --- |
| Transport and connection lifetime | Services/AndroidMidiTransport.cs, AndroidMidiInput.cs, Fm1Connection.cs, PerformanceState.cs, HardwarePerformance.cs | Android port directions and raw-byte semantics are explicit. Incoming Android buffers are copied; outgoing calls serialize with port disposal. Open listeners handle timeout/cancellation and late callbacks. Controller delivery has queue bounds and generation/delivery-epoch checks. Renewal replacement issue above. No physical fragmentation/unplug validation performed. |
| Protocol | EditorClient.cs, EditorCodec.cs, DeviceInfo.cs, PerformanceWire.cs, HardwarePerformanceSession.cs, EditorClient.Performance.cs, EditorClient.UsbPlayback.cs | Requests serialize; ambiguous send/timeout/cancellation faults the epoch. Stream parser bounds payloads and tolerates realtime bytes. Legacy capability probes are version gated. Lease owners serialize press/release/renew. USB return flags/schema/gain/diagnostic ranges are checked. Notification subscribers still execute inside the receive path; no blocking subscriber was proved here. |
| Notes and scheduling | Workstation/Performance.cs, ExternalMidiInput.cs, PatternLoop.cs, ModulationPlayer.cs, CompositionAudition.cs; Services/LivePerformance.cs, PerformanceTransport.cs, DeviceEditing.cs | Shared pitches use reference counts; loop replacement clears held notes and preserves absolute time. Cancellation has outer adapter cleanup in addition to loop cleanup. External MIDI running status/realtime/system-common handling, sustain and source-channel reset are explicit. Note-send exceptions expose the invariant defect above. |
| Native song library and transfers | Fm1SongLibrary.cs, Fm1Operations.cs, SlotImages.cs; song-library caller boundary | Bounded archive reads, project/CRC checks, engine compatibility, durable recovery callback before mutation, dependency-before-project writes and final readback. Native signed optional metadata is retained as raw bytes. Song-order offset 48/36-byte span agrees with firmware `project.c` static assertion; preferences are retained from current settings. UI layout/gesture behavior excluded. |
| Sample conversion and capture | SampleEncoding/SlotBuilder.cs, ConversionPipeline.cs, PcmConversion.cs; Workstation/SamplePreparation.cs, SampleKitPlan.cs; Core/Sampling/PcmWave.cs, CaptureJournal.cs, SampleDocument.cs (save/restore); Services/PcmRecorder.cs, SampleCapture.cs, SampleKit.cs, SampleWorkspace.cs | Fit checks precede conversion, input PCM stays derived, cancellation is checked through long work, and ADPCM previews preserve mapping indices. Recorder uses a bounded nonblocking writer queue, complete frames, selected-route checks, a two-minute limit and recoverable PCM/WAV journal. No physical microphone/USB audio acceptance test or power-loss durability test performed. |
| Session/library persistence | Workstation/Sessions.cs, WorkspaceGenerationStore.cs, WorkspaceFiles.cs, SampleLibrary.cs; Services/SessionWorkspace.cs, SampleWorkspace.SessionAdoption.cs, SampleWorkspace.CrossSessionLibrary.cs | Archive paths/counts/actual streamed bytes are bounded; library reuse revalidates hashes and sidecars, copies privately and publishes original last. Generations stage before pointer replacement with expected-root guard and fallback. Did not prove cross-process filesystem atomicity or Android power-loss guarantees. |
| Offline composition and persistence | OfflineComposition.cs, CompositionDraftStore.cs, CompositionDraftArchive.cs, CompositionAudition.cs; Sequencing/Validation.cs and Models.cs boundary | Strict prompt vocabulary/bounds; draft archives reject duplicate properties, missing fields, extra entries and bad checksum; publish gives imported drafts a new identity. Audition owns a transport epoch. UI prompt request/review ownership is the other reviewer's scope. |
| Hardware sound boundary | Services/SceneHardwareBatch.cs, SoundAudition.cs, relevant DeviceEditing.cs methods | Complete baselines before writes, per-track reconciliation after attempts, connection-token checks and explicit unknown/conflict state. Did not inspect all sound-bank implementations or all SoundDesign generation internals. |

## Executable validation

All successful runs used the private source copy and private `TEMP`/`TMP`, with `DOTNET_CLI_TELEMETRY_OPTOUT=1`, private `DOTNET_CLI_HOME`, build servers disabled and shared compilation disabled. Installed SDK: 11.0.100-rc.2.26504.105, target/runtime net10.0. The song runner's copied project was adjusted only in the private snapshot to reference the original read-only upstream fixture at its absolute path. This was necessary because moving the project changes its relative fixture path.

| Runner | Result |
| --- | --- |
| Sloop.Protocol.Tests | 410 protocol/simulator/sampling checks passed |
| Sloop.Transport.Tests | 26 checks passed |
| Sloop.Fm1Songs.Tests | 25 archive/recovery/order/restore checks passed |
| Sloop.SampleLibrary.Tests | 26 checks passed |
| Sloop.Composition.Tests | 546 focused checks passed |
| Sloop.SampleEncoding.Tests | 102 assertions and 9 Python golden fixtures passed |
| Sloop.ExternalMidiInput.Tests | Decoder, ownership, sustain, channel reset, harmony and timed disconnect checks passed |
| Private failure repro | Both findings above reproduced |

Logs are directly under `build/code-review-android-core-51/`. Initial attempts hit CLI telemetry registry restrictions, build-server startup failure, sandboxed default temp paths and the relocated fixture path; later successful runs resolved these without production edits. Do not interpret those setup failures as application failures.

Test-quality strengths: byte-level external golden fixtures, genuine upstream project fixture, backup-before-mutation assertions, failed durable backup, final corrupted readback and shared-pitch timing/cancellation assertions cover observable invariants. Limits: `SongWire` implements the protocol contract locally and does not reproduce firmware scheduling/flash failure; transport timing tests use wall-clock delays and may be sensitive to a heavily loaded host. They do not establish audio deadlines, Android USB packet behavior or recovery from real I/O ambiguity. The two proposed regressions should add those missing state transitions without mirroring private implementation fields.

## Unmeasured opportunities (not defects or measured savings)

1. `Core/Sampling/PcmWave.cs:Peaks` reads individual samples through `BinaryReader.ReadInt16`; block-based peak code already exists in SamplingTools and is used by cross-session reuse. Consolidating the legacy import/restore peak path may improve large-WAV scan throughput, but benchmark identical peaks, malformed/truncated behavior, cancellation and allocation before changing it. No speedup claimed.
2. `Workstation/PatternLoop.cs` rebuilds/sorts the same event array each phrase and polls with 1 ms delays. Cache events until replacement if profiling shows meaningful allocation/CPU cost. Preserve late queue/cancel visibility and event ordering; do not trade scheduler correctness for fewer wakeups without jitter measurement on Android. No timing improvement claimed.
3. Workspace generations have no pruning policy in the inspected store. A future bounded retention policy could reclaim abandoned/prepared historical copies while preserving active/previous roots and any user work. Establish recovery/retention intent first; this review does not recommend deleting data automatically.

## Explicit limits

This was a targeted core review, not complete inspection of every Android file. Not fully reviewed: UI screens/gestures, all Sequencing editors/history/prompt internals, all SoundDesign engines/generators, all scene arrangement code, sound-bank saving, every service partial, Android MediaPlayer behavior on hardware, SAF provider edge behavior, cross-process concurrent file access, or actual USB/audio latency. No APK build, firmware target build, flash, physical phone test or whole-suite run was performed. Two verified defects and the listed passing checks are not proof that other paths are bug-free.
