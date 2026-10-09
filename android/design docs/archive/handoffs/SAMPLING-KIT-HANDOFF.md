# Sampling to kit handoff — 2026-10-07

Parent integration complete: KitUpload now calls ConfirmProposedSampleSlot; existing stale,
backup/journal and verified upload code is retained. The normal Workstation runner executes
all 79 mapping checks. Combined APK builds and Pixel chop/map/assignment navigation pass.
Physical upload and audible acceptance remain pending. Original wiring guidance follows.

Owned changes are in `SampleEditor.cs`, `Services/SampleKit.cs`, the new workstation
`SampleKitPlan.cs`, and dedicated test files. No other editor, project, solution, firmware,
APK, device installation, commit, or service restart was changed by this slice.

## Implemented

- Directly below the waveform, selected-chop duration, source audition and **Map & prepare
  kit** remain available without expanding precision/navigation/transient controls.
- Send page starts with preparation readiness and source/encoded audition for the selected
  chop. An expandable assignment list shows source chop, note name, MIDI root and inclusive
  key range. Selecting a row auditions its source slice. Drum mappings explicitly ignore
  the first root; instrument mappings explain playback-speed pitch changes.
- `SampleKitPlan.Assignments` derives assignments in source order from the existing mapping
  and firmware-tool midpoint rules. No guessed pad-note map was added. Drum wire zones sort
  by root: encoded audition now resolves `ZonePreview.InputIndex`, rather than assuming
  the source chop index equals its wire-zone index.
- Kit settings reject invalid enums, gain and overflowing root ranges before persistence.
  Loading invalid settings retains defaults with an error status. An unchanged settings save
  preserves a valid conversion. Conversion still clears stale results first and saves the
  derived image through the existing durable `WorkspaceFiles.StoreBytes` mechanism.
- A proposed USR1–4 destination is held in Activity memory and resets on source change.
  This never writes hardware or automatically chooses an unused slot. The current upload
  chooser remains authoritative until the parent hook below is applied.

## Exact parent wiring (KitUpload.cs, outside assigned ownership)

In **AddKitUpload only**, change:

```csharp
button.Click+=(_,_)=>ConfirmSlot("Choose sample slot to replace",
    /* existing message unchanged */,
    async(_,a)=> {
        // existing body
    });
```

to:

```csharp
button.Click+=(_,_)=>ConfirmProposedSampleSlot("Choose sample slot to replace",
    /* existing message unchanged */,
    async slot=> {
        // existing body, replacing every a.Which in THIS callback with slot
    });
```

Keep all existing stale artifact/draft guards, preview stop, capability query, backup,
durable replacement/journal, upload and readback verification code. Do not change
`AddRestoreSlotButton` or `ConfirmSlot`. `ConfirmProposedSampleSlot` is already implemented
in the owned partial: a proposed target is explicitly named in the replacement confirmation;
**Other slot** opens the existing chooser; with no proposal it uses the existing chooser.
Cancel never sends. The helper does not infer device stopped state or claim to stop FM1.

No MainActivity.cs, SceneIntegration.cs, workspace/session adoption or csproj/solution
wiring is required. New workstation source is included by SDK default compile items.

## Validation

```powershell
& './android/src/Sloop.Workstation.Tests/RunSampleKitPlanChecks.ps1'
dotnet run --project android/src/Sloop.Workstation.Tests/Sloop.Workstation.Tests.csproj --no-restore
```

Passed **79 new checks** and **61 existing workstation checks**. New checks compare assignment
plans with actual encoded headers for 1/3/16 chops in all three mappings, exact fit,
unsorted drum lane identity, highest legal roots, invalid enums/ranges/gain and immutable WAV.
The dedicated script creates a temporary runner without editing shared csproj/solution files.
Parent may call `SampleKitPlanChecks.Run()` in the existing runner and retire the script later.

The sandbox .NET workload resolver failed accessing Windows service metadata; standalone
checks passed outside that sandbox. No service was modified. No combined Android build or
Pixel layout/audio/USB acceptance was performed here. Parent should compile the UI and verify
on Pixel 1080×2400: waveform actions, selected source/encoded audition, collapsed assignments,
prepare button, explicit target confirmation, Other slot, cancel and stale-result rejection.
All long assignment rows are inside a collapsible section; primary controls use full-width or
two-column buttons. Backup/readback protections are retained, but physical transfer and audible
FM1 behavior remain unverified.
