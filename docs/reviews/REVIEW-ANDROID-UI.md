# Android presentation and platform review

**Implementation follow-up (2026-10-23): UI-1 through UI-4 are implemented in the shared working tree.** The original review below is retained as the finding record. Final changes, tests and remaining acceptance limits appear at the end of this report.

Reviewed 2026-10-23 against the current working-tree Android sources, not merely HEAD. No production files, shared build products or Git refs were changed. Evidence and source snapshot: `build/code-review-android-ui-51/`. This report owns only the Android presentation/platform scope; firmware optimizations already documented in OPTIMIZATION.md, ARCHITECTURAL-HEADROOM.md and REDUNDANCY-AUDIT.md are not counted here.

## Integration recommendation

Fix the patch-import destination ownership issue first, then make pending exports and asynchronous dialog continuations survive or safely expire across activity recreation. Add Android-level recreation/accessibility regressions. The findings below are source-confirmed control-flow defects; device reproductions remain required. There are no measured size/speed optimization recommendations from this review.

## Framework and coverage

The app is native .NET for Android (`net10.0-android`), using `Android.App.Activity`, programmatically constructed widgets and custom Canvas views. It does not use Compose, MAUI, fragments, a navigation framework or viewmodels. `WorkstationState` selects the workspace/track; `ShowWorkspace` removes and reconstructs content. `Fm1Connection`, `SampleWorkspace` and `EditingWorkspace` retain process-owned state; many pending UI operations instead live in activity fields.

Inspected in detail:

| Area | Files / paths actually traced |
| --- | --- |
| Lifecycle, navigation, event ownership | MainActivity.cs, MainActivity.MidiInput.cs, WorkspaceStyle.cs; AndroidManifest.xml; initialization, start/stop/destroy, saved state, dispatcher delivery and workspace rebuilding |
| Perform | PerformEditor.cs, PerformHardwareEditor.cs, PerformOptions.cs references; chord/voice-leading/part-writing selection, latch, register follow, expanded pads, surface pointer ownership, cancellation, recording completion, hardware hold controls |
| Sampling | QuickSampleEditor.cs, SampleEditor.cs, SampleChopAudioEditor.cs, KitUpload.cs; SampleWorkspace.cs, SampleWorkspace.Quick.cs, SampleWorkspace.ChopAudio.cs, SampleKit.cs; trim hit testing, preview cancellation/routing, retained export, conversion identity, upload backup/verification |
| Sequence and prompts | SequenceEditor.cs, PianoRollEditor.cs, PianoRollView.cs; PatternPromptEditor.cs; EditingWorkspace.cs and sequencing Transactions.cs for proposal/undo ownership |
| Library/import | LibraryEditor.cs, MidiFileEditor.cs, CompositionArchiveEditor.cs, CompositionDraftEditor.cs; selected session integration/adoption and scene subscription/boundary paths in SessionIntegration.cs and SceneIntegration.cs |
| Tests | PianoRoll.Tests project and runner, PerformanceControls.Tests, PartWriting.Tests, selected Workstation.Tests and Sequencing.Tests; Test-Domain.ps1 discovery/build behavior; selected runner project dependencies |

## Ranked confirmed findings

### 1. [P2] Restored SysEx picker results can overwrite the wrong synth

References: `android/src/Sloop.Android/LibraryEditor.cs:30`, `:38-41`, `:67-71`; `android/src/Sloop.Android/MainActivity.cs:185-190` and activity declaration at `:11`.

Trigger: select Synth 2 or 3, start **Import DX voice or bank SysEx**, recreate the activity while the Android document picker is open (rotation is enough), then return a valid single voice. `importSoundTrack` belongs to the original activity and is not saved. The replacement activity defaults it to zero even though the selected track itself is restored. `FileResultAsync` therefore applies the patch to `editing.Sounds[0]`. A bank import likewise captures the wrong track in its subsequent voice chooser. This changes and saves an unintended local patch; sound Undo offers recovery but does not prevent the wrong-target edit.

Minimal fix: store a pending-import descriptor with an explicit nullable target, workspace identity and request identity outside the activity or in saved state. Consume it only when valid; if it cannot be restored, reject with a visible retry message rather than defaulting to track zero. Revalidate the workspace at application time, as the MIDI/composition import paths already do.

Meaningful regression: begin imports for each synth, recreate the activity before returning a single voice and a bank, and assert that only the captured destination changes. Also return a result without a descriptor and assert no patch changes. Current host tests cannot instantiate this activity/result lifecycle and therefore miss the issue.

### 2. [P2] Recreating the activity during export silently drops the export payload

References: `android/src/Sloop.Android/LibraryEditor.cs:11`, `:23-35`, `:48-52`; `MainActivity.cs:185-190`. Related explicitly expiring path: `CompositionArchiveEditor.cs:16`, `:26-35`, `:49-52`.

Trigger: export a patch, recovery file or session, recreate the activity in Android's create-document picker, then select a destination. `exportBytes` and `sessionExportPath` are activity fields and are not restored. The new activity sees both null and silently returns from the successful result handler. No bytes are written to the chosen document; a provider-created empty document can remain. Session export's private cache file also loses its cleanup reference. Composition exports lose their payload too, but at least report expiration explicitly. In contrast, WAV/kit pending export lives in process-owned SampleWorkspace and survives ordinary activity recreation.

Minimal fix: retain the pending export descriptor outside the activity and use a private staged file for potentially large byte payloads; persist its reference/request identity as needed. Consume and clean it on completion/cancel. An unavailable descriptor must produce an explicit retry outcome rather than silent success handling. Do not put large exports in a Bundle.

Meaningful regression: export SysEx, a slot image and a session archive through a fake document provider, recreate before completing selection, then assert byte equality and cache cleanup. Test cancel/retry and unavailable descriptor paths. Existing archive/codec tests validate bytes before the Android picker and do not cover descriptor survival.

### 3. [P2] Sample preparation and pitch-estimate continuations can show dialogs on a destroyed activity

References: `android/src/Sloop.Android/QuickSampleEditor.cs:87`, `:115-128`; `SampleChopAudioEditor.cs:15-20`; lifecycle cleanup in `MainActivity.cs:144-157`. Similar generation continuations exist at `PatternPromptEditor.cs:105` and `:143`.

Trigger: start **Send to FM1** preparation or **Estimate pitch / root**, then rotate or finish the activity before the background work completes. Both handlers await process-owned service work and then unconditionally construct/show an AlertDialog using the original activity. `OnDestroy` requests conversion cancellation, but cancellation cannot retract an already completed task/queued continuation; pitch estimation is not canceled there. The quick-send task is discarded (`_ = ...`) without an outer catch. The estimate click uses async void without a catch around dialog creation. Showing a dialog after activity destruction can fail with an invalid window token; the estimate event can surface an unhandled exception, while quick-send can leave an unobserved task failure and no actionable review.

Minimal fix: after each await, verify activity lifetime and the captured source/workspace/request identity before showing UI. Cancel or invalidate activity-owned requests on teardown. Retain useful conversion results in the service and let the current activity reopen review. Catch failures at async event boundaries. Use the guarded composition-archive/MIDI import continuations as local examples rather than introducing a generic task framework.

Meaningful regression: block the preparation/estimate service behind a controllable completion, destroy/recreate the activity, release completion, and assert no old-window dialog, no unhandled exception and no upload. Then reopen the current activity and verify a valid result can be reviewed. Host DSP/conversion/pitch tests do not exercise window-token lifetime or event-handler exception delivery. This is source-confirmed missing ownership protection; no device crash was induced during this review.

### 4. [P2] Accessibility activation cannot operate the performance controls

References: `android/src/Sloop.Android/PerformEditor.cs:244-270`; `PerformHardwareEditor.cs:42-65`.

`PerformanceSurfaceView` draws every pad/key itself and exposes one generic content description. It has no accessible child nodes or per-pad actions; `PerformClick` calls only the base implementation and emits no performance event. TalkBack/switch-access click activation therefore cannot select or play any of the labeled pads. HardwareGestureButton is a labeled Button, but its performance actions exist only in `OnTouchEvent`; its accessibility click similarly invokes no GesturePressed/GestureReleased action. A Release/stop button does not provide an accessible way to start these gestures.

Minimal fix: expose named per-pad virtual accessibility nodes/actions that emit a bounded press/release through the same owner-safe path, and define a bounded accessible activation for hardware buttons. Keep raw multitouch behavior intact and ensure accessibility actions always release their owned notes/effects. This addresses an inaccessible existing control, not a proposed new musical feature.

Meaningful regression: invoke ACTION_CLICK through AccessibilityNodeInfo for a named pad and hardware effect, assert the expected onset and guaranteed release, then exercise cancel/background/disconnect. Confirm labels/destinations with TalkBack. Current domain tests feed performance engines directly and never use Android accessibility actions.

## Clean areas and intentional behavior

- Performance cells use pointer IDs, explicit release on pointer-up/outside hit testing, cancellation handling, and ignored-pointer suppression after external stop. The scroll rail and split-motion parents are deliberate accommodations for playing while scrolling. No change is recommended merely because this differs from ordinary scrolling widgets.
- Lifecycle paths release app-owned performance, macros and modulation, stop playback/preview and close controller input. Controller delivery includes connection/delivery generations and a bounded pending queue. Connection changes synchronize controller destination identity. These are useful existing protections, not new optimization opportunities.
- Latch holds the prior register until the next fresh chord; register changes clear voice-leading history. Explicit upper pad octaves, inversions and part-writing fallback are intentional. The review does not classify their musical policy as defects.
- Local pattern edits fork, validate, save, then replace history. Proposal application requires the captured snapshot, preventing stale UI reviews from silently replacing later pattern edits. Prompt reviews call this path. Host tests include actual stale rejection and undo/redo content invariants.
- MIDI and composition archive imports capture source/workspace identity and revalidate before application. MIDI hardware transfer also checks destination-session identity. Slot upload validates artifact identity, backs up the target, records a pending transfer and verifies readback before reporting success.
- Sample previews check an epoch after background preparation; explicit routing is verified and output loss stops playback. Chop previews currently build at the matching default 22050 Hz; the missing explicit sample-rate argument at that call is not itself a demonstrated pitch bug.

## Test quality and validation

Private snapshot copied Android sources excluding bin/obj; no builds were run against shared source output directories. Private logs retain results.

- `Sloop.PianoRoll.Tests`: **87 focused checks passed** in the private snapshot (`Sloop.PianoRoll.Tests.retry.log`). These cover actual behavior including canceled drawing, identity-preserving move, undo, resize limits, stale rejection and protection, rather than only asserting source spelling.
- Attempted PerformanceControls, Composition, MidiFile and QuickSampling runners: initial CLI initialization failed because telemetry attempted disallowed registry access. Retrying with telemetry disabled/private CLI home allowed piano-roll execution, but the other selected builds failed without compiler diagnostics. A further `--disable-build-servers -v normal` attempt ended in Restore with 0 warnings/0 errors and failed status (`build-diagnostic.log`). These are **inconclusive environment/build outcomes**, not failing behavioral assertions and not passing tests.
- PerformanceControls tests exercise MIDI messages, owner refusal, reset failures and value bounds. PartWriting tests include musically meaningful parallel-motion/leading-tone invariants as well as exact voicing examples. Exact-note expectations are appropriate for stable musical policy, but alone cannot validate the Android latch/joystick/register wiring.
- Workstation's test labeled "latched octave change" directly changes a LiveNoteSet owner. It does not execute MainActivity's policy of keeping the sounding latched register until the next chord. Retain the engine invariant, and add an activity integration test for the actual policy; do not infer UI coverage from the label.
- PianoRoll.Tests links PianoRollModel, not PianoRollView. It validates model transactions but cannot establish Android multi-pointer dispatch, accessible actions or detached-view lifecycle behavior.

## Unmeasured optimization / maintenance opportunities

1. `WorkspaceStyle.cs:54-64` eagerly builds collapsed sections. `ShowWorkspace` then rebuilds the entire tree for many service notifications. Profile render duration, allocations and file reads on a representative sample/large pattern before changing it. A narrowly lazy section builder or coalesced notifications may help; this is an unmeasured opportunity with no APK/RAM/speed claim. Preserve field drafts, event ownership and scroll restoration when evaluating it.
2. `Services/SampleKit.cs:17` computes CurrentKey using ChopOptions, whose getter reads/parses a sidecar file. Repeated render-time Converted/QuickConverted queries can repeat file I/O. Measure read counts before considering a cache keyed to document/settings revision with explicit invalidation. Do not weaken corrupt-setting validation.
3. The partial Activity centralizes UI and process-service coordination. The concrete fixes above need small request descriptors and lifecycle guards, not a wholesale architecture replacement. Extract only ownership policies that can be regression-tested independently.

## Explicit limits

This was a source review with one successful private host runner, not an emulator/hardware acceptance pass. No actual Android rotation, document provider, TalkBack, multitouch or USB detach session was driven. No APK size, rendering latency, GC, audio deadline or device CPU measurements were taken. Scene scheduling, connection/protocol internals, all service persistence failure modes and every library/editor dialog were not exhaustively audited; core/protocol has a separate owner. No firmware behavior, musical content, partition, runtime decompression or protocol changes are proposed. Clean areas describe inspected protections, not proof that those areas contain no bugs.

## Implementation handoff — UI-1 through UI-4

### Changes now in the working tree

- **UI-1:** `UiRequests.cs` provides a small disk-backed import descriptor containing request ID, captured synth destination and workspace identity. `MainActivity.UiRequests.cs` binds only IDs to Android saved instance state. `LibraryEditor.cs` validates the descriptor after reading and again before applying a selected bank voice; absent, superseded, destroyed-activity or changed-workspace requests reject explicitly. There is no default-to-Synth-1 result path. Requests are cleaned on consumption/cancellation.
- **UI-2:** Patch, recovery-file, session and composition exports now share staged export ownership. Payload bytes stay in app-private files, with request ID/name/MIME retained separately; only IDs enter the Bundle. Successful provider writes flush and then clean staging; cancellation and explicit discard clean it; write failures retain the prepared bytes for Retry or the Library's retry/discard controls. Missing descriptors produce an explicit retry instruction. Abandoned, unreferenced requests older than seven days are pruned at startup while saved pending IDs are retained. No large byte arrays are put in saved state. Existing WAV/kit service-owned pending-export paths are not redesigned here.
- **UI-3:** Activity-bound request generations invalidate sample and composition continuations on stop/destroy. Quick preparation and pitch review recheck lifetime, request, document identity/revision, workspace and relevant selected mode/chop/settings before showing a dialog or applying a result. Quick send also captures/rechecks the destination session. Composition generate/regenerate continuations use the same lifetime/source rule. Async sample event/review boundaries and the activity-result boundary catch failures. Ordinary service-driven view rebuilding does not invalidate an otherwise valid request.
- **UI-4:** Added native, individually named per-pad Buttons in an Accessible pads section, automatically expanded for touch exploration. They cover the entire expanded pad range and update labels on register changes. Each uses its own performance owner, bypasses latch only for that bounded activation, and releases after 350 ms or earlier stop/background/disconnect. The raw Canvas surface stays available for multitouch but is omitted from accessibility focus, avoiding a nonfunctional generic click target. HardwareGestureButton now maps accessibility/keyboard click to a bounded press/release; raw pointer-up calls the base click without starting an additional hold. Next-bar-fill activation holds for one app-tempo bar plus a margin; hardware transport timing is not inferred or changed. `UiBoundedGesture` installs cleanup before onset and uses activation generations so an old timeout cannot release a newer activation, even when delegates are reused.

Raw playing surfaces, scale/chord mapping, joystick behavior, musical engines, factory content, protocol interfaces and the existing latched-register policy remain in place. No shared service/core-owned files were edited by this UI batch. No commit, version change, package generation, publication or hardware action was performed.

### Regression and compile evidence

All build products and baseline variants are under `build/code-review-android-ui-51/implementation/`.

- **70 checks passed** in the new `android/src/Sloop.AndroidUiOwnership.Tests` runner (`domain-run.log`). It links the actual production request store, activity saved-state/request binding, native accessible-pad handler file and hardware-button implementation against a minimal Android platform shell. Tests cover all three restored import destinations, changed/missing/consumed request rejection, exact export bytes after store/activity recreation, provider-write failure/retry, cleanup/cancel/partial staging/orphan expiry, source/lifetime invalidation, native expanded-pad action mapping and labels, automatic/early release, failed onset cleanup, repeated activation, stale-timeout suppression, raw touch-up suppression and disabled/detached hardware actions. These are host-boundary tests, not Android framework instrumentation.
- Existing private regressions also pass: **PianoRoll.Tests 87 checks** and **PerformanceControls.Tests 3,035 checks including modulation** (`Sloop.PianoRoll.Tests.run.log`, `Sloop.PerformanceControls.Tests.run.log`).
- **Failing before / passing after:** the same hardware-button assertions compiled against the pre-fix `PerformHardwareEditor.cs` fail at `Accessibility ACTION_CLICK dispatches a real hardware press` (`baseline-run.log`); current production dispatch passes. UI-1/2/3 recreation/continuation regressions exercise the new production ownership boundary and saved-state binding, but were not run against an Android emulator or the old full activity.
- **Full app C# reference compilation passed, 0 warnings / 0 errors** (`android-reference-build.log`). In the private copy only, Android projects were compiled as net10.0 libraries against installed `Microsoft.Android.Ref.36/36.1.69` Mono.Android/Java.Interop assemblies, with the SDK's usual Android.App global using supplied privately. All Android sources and their project dependencies compiled. This validates C# and Android API signatures; it is not APK packaging/linking validation.
- A normal private `dotnet build Sloop.Android.csproj` attempt **could not produce an APK**: the usable SDK is `11.0.100-rc.2.26504.105`, and build reports **NETSDK1147: the android workload must be installed** (`apk-build.log`). An attempted pin to 10.0.401 was not recognized as an installed usable SDK despite the directory being present (`domain-build` initial attempt; final log supersedes it). No SDK/workload installation or system configuration change was attempted.
- Original review-only logs remain separate above this implementation directory; earlier inconclusive build results should not be confused with these successful final host/reference builds.

Reproduction from a private source copy: `dotnet build android/src/Sloop.AndroidUiOwnership.Tests/Sloop.AndroidUiOwnership.Tests.csproj --disable-build-servers -m:1 -p:UseSharedCompilation=false`, then execute its net10.0 DLL. The shell runner has no Android workload dependency. It is discovered automatically by the existing Test-Domain.ps1 project discovery; that shared script was not edited.

### Parent integration hooks and acceptance limits

No code hook remains blocked by ownership. Include the three new production files (`UiRequests.cs`, `MainActivity.UiRequests.cs`, `MainActivity.Accessibility.cs`), the dedicated test project and the edited presentation partials with this batch. The separate Android-core fixes remain owned by their implementer.

Run a normal APK build on a host with a matching Android workload before release. Actual rotation/process recreation with a document provider, real window-token delivery, TalkBack/switch-access traversal, simultaneous physical touches and USB detach remain acceptance checks; none are claimed as driven here. The host shell does not prove Android framework event ordering. Staged request recovery handles restored request IDs; if the OS discards saved state, requests expire safely and orphan files are eventually cleaned. Accessibility notes are deliberately bounded rather than latch-toggling; raw latch/register semantics are unchanged. No APK size, rendering speed or hardware deadline improvement is claimed.
