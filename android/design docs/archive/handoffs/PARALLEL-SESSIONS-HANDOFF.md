# Sessions and performance presets handoff — 2026-10-07

## Completed owned slice

- New `Sloop.Workstation/Sessions.cs`: SessionManifest/SessionAsset/SessionInput/SessionCandidate,
  immutable named SessionStore snapshots, hashed portable archives, bounded staged validation,
  durable activation pointer with previous pointer, saved performance presets.
- New `Sloop.Android/Services/SessionWorkspace.cs`: captures saved workspace documents,
  original patch imports, referenced sample original/edits/conversion settings/image, current
  performance settings and saved presets. Validates application content before publication.
- New `Sloop.Android/SessionPresetCodec.cs`: explicit codec for existing PerformOptions;
  enum/range validation before applying. No existing domain model was redefined.
- Owned `LibraryEditor.cs`: named save, stream-based archive import/export through Android
  document picker, name filtering, session counts/dates, saved preset save/apply, retained
  grouped backup/import/document browser. Load is enabled only when adoption hook exists.
- New independent `Sloop.Sessions.Tests` console project; owned STORAGE.md updated.

No reserved shared files, existing tests, solution, firmware or device installation changed.

## Exact shared-shell integration

1. Register `android/src/Sloop.Sessions.Tests/Sloop.Sessions.Tests.csproj` in Sloop.slnx.
2. In MainActivity initialization assign `AdoptSessionAsync = candidate => ...`.
   The Library handler already calls `SessionWorkspace.Prepare(id)` and validates all stored
   assets before invoking this callback. It calls `SessionStore.Activate(id)` only on success.
   The callback must fail before adopting anything if preparation fails. It must stop transport,
   capture and owned notes, prepare all three SoundDocuments/sequence/native/sample documents,
   then durably install the generation and adopt the prepared editor state together.
3. Preferred workspace architecture: add a shared durable active-generation pointer and resolve
   editor persistence paths through it; seed a writable generation from candidate.Directory,
   flush all files, replace that pointer, then swap prepared editor references. Keep the previous
   generation. Do not copy documents one by one over the current live workspace. The existing
   EditingWorkspace and SampleWorkspace hardcode app-directory paths, so this requires their
   owner to provide reload/path-resolution hooks. This slice deliberately does not mutate them.
4. The callback may load `settings/performance.json` with JsonSerializer.Deserialize of
   Dictionary<string,string>, then SessionPresetCodec.Decode; assign performOptions only after
   transactional adoption. Presets are at `presets/preset-{id:N}.json`; stage their installation
   into sessions/preset-{id:N}.json as part of adoption if wanted. Candidate asset paths are
   relative to candidate.Directory; sample current.txt references its original filename.
5. Scene component passes its IDs and portable document assets via
   `SessionWorkspace.Save(name, scenes, presets, extraAssets)`; capture consistent scene state
   in the shared save coordinator. No scene format is invented here. Existing Library named
   saves currently pass stored preset IDs, and capture all stored presets and saved settings.
6. For startup restoration use `SessionStore.Current`, validate content, and route through the
   same adoption coordinator. A corrupted current pointer should surface recovery via
   `current.previous`, never replace live defaults silently. Current getter intentionally throws.

The store's `Save` and `Import` optional `Action<SessionCandidate> validateContent` executes
inside staging before immutable publication. `Open/Prepare` do not alter current work.
`Export(Guid, Stream)` leaves the platform output open. Import accepts nonseekable streams
without unbounded buffering. Name collisions always produce new IDs. Snapshots are retained.

## Validation and remaining acceptance

Commands from repository root:

```powershell
dotnet restore android/src/Sloop.Sessions.Tests/Sloop.Sessions.Tests.csproj -m:1 -p:RestoreUseStaticGraphEvaluation=true
dotnet build android/src/Sloop.Sessions.Tests/Sloop.Sessions.Tests.csproj --no-restore -m:1
dotnet android/src/Sloop.Sessions.Tests/bin/Debug/net10.0/Sloop.Sessions.Tests.dll
dotnet build android/src/Sloop.Android/Sloop.Android.csproj --no-restore -m:1 -t:Compile "-p:AndroidSdkDirectory=C:\Users\shaun\AppData\Local\Android\Sdk" "-p:JavaSdkDirectory=C:\Users\shaun\AppData\Local\Android\Jdk"
```

32 checks pass; standalone domain/test build passes with zero warnings/errors. Android managed
compilation passes with zero errors; the latest build reports four unrelated CS0108 warnings in concurrent Fm6Visuals.cs. Full packaging hit APT2000 (AAPT2 pipe being closed),
after concurrent sampling-editor compilation stabilized. No hardware tests or APK installation.

Limits: List validates/hashes all stored snapshots synchronously before the UI displays up to
40 matching items. Large saves and archive construction run on the UI thread; imports and
destination-stream export copies run on workers. No schema migrations/deletion/automatic
orphan-staging cleanup are implemented, and only the current
sample plus sidecars are automatically captured. Unknown schemas are rejected while source
archives stay untouched. Explicit future asset references can use extraAssets. Shared-shell
atomic editor adoption, startup reload and coordinated scene capture are integration work.

The 32 automated checks cover rejected staged content and failed pointer activation; they do
not verify live editor adoption, physical low-storage/power-loss recovery, hardware behavior,
or every maximum-size resource limit. STORAGE.md records those acceptance boundaries.

