# Offline prompt expansion handoff

Parent integration complete: Sequence uses this helper with channel/bar/step selection,
selected-step locks and full change review. Native selected-step commands are wired too.
Combined Android build and Pixel proposal/discard verification pass. Instructions below
describe the original slice and remain useful for extending the caller contract.

2026-10-07. This slice is deterministic C# command interpretation over existing content.
No conversational model, Android prompt panel, sample prompt engine or hardware audition
was added. Reserved Android editors and project/solution files were untouched.

## Available helper

`Sloop.Sequencing.PromptEditor` is a standalone platform-neutral helper in
`src/Sloop.Sequencing/PromptEditor.cs`. Both `Propose` overloads return `EditProposal`.
Callers supply the actual immutable source snapshot, selection, prompt, explicit locks,
optional integer seed and cancellation token. Proposals contain complete before/after
states, typed diffs and `pattern-phrases/1` provenance with the original prompt.
Planning never mutates the source. Cancellation before or after domain planning throws
without returning/applying a proposal. Revision IDs are fresh; musical content for the
same source/selection/seed is deterministic.

## Exact vocabulary

Commands ignore case and normalize whitespace; otherwise match the whole string.
Use one command per proposal, at most 256 characters. `N` is a positive 32-bit integer;
transpose is at most 127 and velocity at most 126. Domain validation rejects any result
outside pitch, velocity, duration, pattern, tie/slide, lock or capability bounds.

| Target | Commands |
| --- | --- |
| App notes | `transpose up N semitones`, `transpose down N semitones`, `move earlier N ticks`, `move later N ticks`, `resize longer N ticks`, `resize shorter N ticks`, `quantize N ticks`, `velocity up N`, `velocity down N`, `simplify` |
| Native pattern | `transpose up N semitones`, `transpose down N semitones`, `move earlier N steps`, `move later N steps`, `velocity up N`, `velocity down N`, `quantize microtiming`, `level normal`, `level ghost`, `level soft`, `level hard`, `simplify` |

Native drum lanes accept level and simplification; pitch/velocity reject. Native synth
thinning rejects because of ties/slides. Lane-only move and microtiming edits reject because
timing belongs to the whole step. Native moves preserve parameter-lock references and reject
overwrites. App selection must contain an existing note inside the optional onset range.
Explicitly empty drum-lane selections reject; native ranges with no active hits can produce
an unchanged proposal. A no-op creates no undo entry.

`simplify` means keep approximately every second selected app event in source order, with
seed as phase. Native drums retain hits according to step/lane/seed parity. It is thinning,
not musical inference or fill generation. Select hats explicitly to preserve kick/snare;
there is no lane-name inference. Locks are hard constraints: edits that would change locked
material reject, rather than silently excluding it. Show this interpretation before Apply.

Unsupported extra clauses, negation, named parts, percentages, syncopation, fills and
reharmonization reject entirely. Supply selection and locks through controls; phrases such
as `simplify but keep kick` reject. No automatic fallback or partial execution exists.

FM6 `PhraseParser` now similarly rejects unrecognized residual wording, competing families,
or opposite directions for one dimension, returning no refinements or locks and
`HasSupportedRequest=false`. Its existing explicit preservation phrases remain supported.
The established phrase `a glassy bell that becomes brighter when I play harder` is fully
recognized. Callers must check `HasSupportedRequest` before Generate/Refine as the current
sound UI does.

## Android wiring for the owning editor

1. Add a compact panel labelled **Offline pattern commands**; display the applicable
   `AppCommands` or `HardwareCommands` string plus selection/range, locks and simplification
   interpretation. Avoid labelling this mode conversational AI.
2. Capture `editing.Sequence.Current` as `AppPattern` (or `editing.Native.Current` as
   `HardwarePattern`) and the editor's explicit selection when the user presses Propose.
   Call the corresponding `PromptEditor.Propose`; display `EditException` or cancellation
   and clear any obsolete pending proposal. Keep the returned snapshot and proposal together.
3. Render each `NoteChange` with old/new pitch, start, duration and velocity; for native
   `TrackChange`, enumerate changed steps/lanes and lock positions from the full states.
   Display provenance and distinguish an unchanged result from an actionable proposal.
4. Invalidate the pending proposal on selection, locks, target or manual-edit changes.
   Apply via `editing.EditPattern(pending)` or `editing.EditNative(pending)`. Those existing
   methods fork history, enforce the exact parent snapshot, save the candidate, then adopt it.
   Do not bypass them by assigning `After` directly. Catch stale-parent/save failures and
   retain the accepted state. Discard only clears the pending proposal.
5. Use existing local undo/redo commands. A proposal is one history entry; redo restores
   stored content without re-parsing. Hardware sending remains a separate explicit operation.
   No new preview backend is supplied; expose only the existing editor's supported audition.

No edits to MainActivity.cs, SceneIntegration.cs, SoundEditor.cs or SampleEditor.cs were
needed. The helper is compiled by the sequencing SDK's existing source glob; no csproj
change is required. Full source/revision history remains session-local as before.

## Validation

Run `dotnet run --project android/src/Sloop.Sequencing.Tests` and
`dotnet run --project android/src/Sloop.SoundDesign.Tests` from the repository root.
New scenarios cover exact parsing, refusal corpus, units/bounds, selection and lock
preservation, cancellation, deterministic native thinning, provenance, stale apply and
full undo/redo. No combined APK, installation, firmware flash or service restart is required.

Next work should implement actual rhythm operations before admitting syncopation/fill
phrases, and wire the panel through the owning editor. Sample trim/chop proposal contracts
and richer follow-up drafting remain separate work; do not advertise them through this helper.
