# Sound audition handoff — October 8, 2026

## Integration follow-up

SoundAuditionRetention.Current now holds the three original baselines for the app process.
SoundEditor no longer owns Activity-local records or captures its Activity in retained adapters.
Root integration calls InitializeSoundAuditions after editing workspace initialization and
OnSoundAuditionWorkspaceReplaced immediately after session adoption. The process-level static
connection observer remains subscribed across Activity destruction; no disposal hook is needed.
Sound screens show retained originals and recovery navigation. Replaced sessions disable B/Keep
until the old audition is restored/finished or abandoned. Disconnect is a sticky Conflict that
preserves the original and prevents late successful operations from overwriting the conflict.
Reconnecting still requires explicit abandonment; no stable physical identity claim is made.
Recovery after process termination remains unimplemented. The dedicated runner passes 40 checks,
including Activity-style singleton reacquisition, session replacement, baseline restoration,
disconnect, busy abandonment rejection and disconnect during an in-flight write.
The integration hooks supersede the original pending work below.

## Implemented

Reversible single-track FM6 RAM A/B using acknowledged complete original baselines,
sequential guarded writes and readback. Original includes patch, macros, preset and PTCH.
Restore/Keep never mutate local SoundDocument or prompt/manual history. B and Keep reject
changed local target/revision. Restore remains independent of local changes. Keep explicitly
means RAM only; bank saving is not provided. Phrase playback reuses owned MIDI preview.
Unknown/partial results require read/reconciliation; external edits block further writes.
Lost ACK may count as applied only when the batch adapter subsequently verifies full state.
Reconnect is treated as conflict even if content matches, since device identity is unproven.
Abandonment explicitly leaves current device RAM untouched.

## Changed files

- src/Sloop.Android/SoundEditor.cs
- src/Sloop.Android/Services/SoundAudition.cs (new pure controller)
- src/Sloop.SoundAudition.Tests/Sloop.SoundAudition.Tests.csproj (new dedicated runner)
- src/Sloop.SoundAudition.Tests/Program.cs
- design docs/SOUND.md and AI-SOUND.md
- design docs/NEXT-SOUND-AUDITION-HANDOFF.md

Paths above are relative to android/. No shared connection, batch, central runner, solution,
session or MainActivity source edits were made.

## Integration hooks and uncompleted work

The current SoundEditor instance owns three SoundAudition references. Parent should retain
these in a process-scoped service across Activity rotation and staged session adoption, with
an explicit recovery indicator when a workspace is replaced. Do not silently discard a live
baseline or automatically restore over a changed local/hardware target.

Add a public opaque physical-device identity + connection generation contract to Fm1Connection,
and notify a retained audition service when disconnected. Current reconciliation compares
SceneHardwareBaseline.Connection internally and refuses writes after reconnect. For recovery
on the same physical device, expose a public adapter that proves identity, double-reads current
engine/preset/PTCH/patch/macros, and issues a new guarded baseline only when the snapshot is
exactly original or the last candidate. Do not refresh fingerprints by blind ReadSoundAsync
and then assume the earlier target is still safe. Current implementation intentionally leaves
RAM untouched and requires abandonment after a reconnect conflict.

Existing public ReadSceneSoundsAsync / ApplySceneSoundsAsync are used with one track only.
Preserve their complete guards, connection epoch checks, double reads and per-track outcomes
when the hardware-scene owner extends those APIs. Controller callbacks accept these existing
contracts; no shared edits or new transport hooks are required for the current slice.

UI currently disables ordinary RAM Apply during an active audition. Local editing remains
available; afterward B/Keep requires finishing/abandoning and capturing a new baseline.
Add dedicated Activity lifecycle compile/UI validation in the parent's serialized Android
integration. No APK build/install performed here. No flash bank saving or atomic write claim.
No additional hardware test gate requested; hardware acceptance remains as recorded in STATUS.

## Validation

Run: dotnet run --project android/src/Sloop.SoundAudition.Tests/Sloop.SoundAudition.Tests.csproj
Result: 20 focused checks passed. Runner links the pure controller and existing batch source
without changing central tests. Checks cover A/B/Restore, RAM Keep, stale identity/revision,
external PTCH changes, lost acknowledgment plus successful readback, partial macro writes,
reconnect conflicts and note-stop callback use. This validates orchestration, not Android UI
compilation, physical synthesis/listening or transport reliability.
