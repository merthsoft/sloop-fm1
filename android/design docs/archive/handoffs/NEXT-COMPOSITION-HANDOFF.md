# Offline composition handoff — October 8, 2026

Implemented a complete bounded prompt-to-editable-symbolic-loop slice. Existing recipe
commands remain available. No shared integration, solution, central runner, firmware,
APK or device actions were performed. Other uncommitted work was preserved.

## Changed files

- `android/src/Sloop.Workstation/OfflineComposition.cs` — new procedural domain engine,
  strict parser/intent/draft, per-note draft edits, part-preserving regeneration and apply proposal.
- `android/src/Sloop.Android/PatternPromptEditor.cs` — owned visible composition entry,
  draft review/note editor/keep/regenerate and existing final proposal review/history integration.
- `android/src/Sloop.Composition.Tests/Sloop.Composition.Tests.csproj`, `Program.cs`,
  `.gitignore` — dedicated domain runner, independently runnable without solution edits.
- `android/design docs/COMPOSITION.md` — actual vocabulary, behavior, tests and limitations.
- This handoff.

## Integration hooks

No project reference is required: SDK default source inclusion builds the new engine into
the existing Workstation project, which Android already references. `DescribePatternCommands`
now includes **Compose a new offline loop**, so the existing Sequence editor entry reaches
composition without editing `SequenceEditor.cs` or `MainActivity.cs`.

Public API in `Sloop.Workstation`:

```csharp
var intent = OfflineComposition.Parse(prompt);
var draft = OfflineComposition.Generate(prompt, source.TicksPerQuarter, cancellationToken);
draft = OfflineComposition.EditNote(draft, note.Id, editedNote); // null deletes
draft = OfflineComposition.Regenerate(draft, keptParts, cancellationToken);
var proposal = OfflineComposition.ProposeApply(source, draft, optionalEditLocks);
editing.EditPattern(proposal); // existing captured-snapshot check, persistence, one undo step
```

`CompositionDraft` contains original text, actual resolved intent (including current seed),
and an editable `AppPattern`. All generated notes stay local until ordinary application
playback/send actions. Channel ownership is fixed: zero-based bass 0, chords 1, melody 2,
drums 9. Requested channels are replaced, others retained, phrase length changes explicitly.
The final review shows channel numbers and before/after loop lengths. Keep selections
survive note editing and regeneration; failed draft edits/regeneration return to the
preserved draft. The implemented vocabulary and remaining product design are documented
in [COMPOSITION.md](../../COMPOSITION.md).
Parent may expose `DescribeOfflineComposition(currentPattern)` directly from a top-level
Sequence action later, or add serialization/audition via the public draft API. Session codec
and shared workspace modifications were intentionally left to parent ownership.

## Validation

`dotnet build android/src/Sloop.Workstation/Sloop.Workstation.csproj --no-restore` passed
with zero warnings/errors. Dedicated runner build passed with zero warnings/errors:

```powershell
dotnet build android/src/Sloop.Composition.Tests/Sloop.Composition.Tests.csproj -m:1 -p:RestoreDisableParallel=true
dotnet run --project android/src/Sloop.Composition.Tests/Sloop.Composition.Tests.csproj --no-build --no-restore
```

Result: **421 focused checks passed**. Includes 126 key/scale/density/bars configurations,
strict malformed-input rejection, exact deterministic notes/IDs, melodic scale membership,
drum pitches, editing/deletion, selective keep/all-keep, seed overflow, cancellation,
protected identities, requested-channel replacement, shorter-loop rejection, timing mismatch,
stale snapshot rejection, one-step undo/redo including loop length, and unchanged edit recipes.
The initial parallel restore returned an empty MSBuild failure; serialized restore/build
above resolved it. No new packages are required.

## Limits and uncompleted work

Android editor source has been reviewed but not compiled or device-tested in this slice;
parent owns the serialized Android build. Hardware validation is accepted and is not a gate.
No new hardware claim is made. Draft review has a note selector and numeric editor rather
than piano-roll gestures. No pre-apply audition, native draft conversion, draft/session
serialization, freely phrased interpretation, tempo/swing controls or audio/model generation.
Fixed I–IV–V–I progression and GM kick/snare/hats are intentionally simple. Pentatonic
chords/bass inherit existing major/minor harmony mapping. Keep choices persist across note
edits and regeneration. Domain tokens cancel generation/regeneration,
but Android has no cancel-in-progress control. Original prompt plus current seed is stored
in provenance; individually kept parts do not retain a per-generation provenance ledger.
Requested-channel replacement is clearly labeled; note/part locks are available in the
domain API, while the composition UI does not add new shared protected-note integration.
History cannot apply a length-only proposal without note differences; the engine rejects
that edge case explicitly. Unsupported prompts always reject; no silent inference fallback.
