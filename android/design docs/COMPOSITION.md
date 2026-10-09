# Offline prompt composition

Current delivery: [STATUS.md](STATUS.md). Build and test evidence: [VERIFICATION.md](VERIFICATION.md).

Status: bounded procedural loop composition implemented October 8, 2026 in
`Workstation/OfflineComposition.cs`, with a visible entry in `PatternPromptEditor.cs`.
The domain build and 546 focused checks pass. Combined Android compilation and APK packaging
pass; earlier Pixel draft-generation/review checks cover the initial composition slice.
See VERIFICATION.md for current evidence and deployment limits. No language-model
runtime is implemented. FM6 sound prompts remain separate. Scene/arrangement prompting
remains planned.

Build, test and deployment evidence is recorded in
[verification record](VERIFICATION.md). This checkpoint records the
implemented composition slice; the broader experience below remains future design.

## Draft audition, saving and controls — October 8, 2026

Review provides MIDI draft audition through the existing loop player, explicit Stop, and
named local Save/Open/Delete. Closing review, leaving the app, changing track/workspace,
or adopting a session stops only the draft's owned transport epoch. The transport epoch is reserved before worker startup, and cleanup clears ownership before
notifying transport callbacks. Existing playback must stop before audition; a failed audition does not acquire another loop's ownership. Generic
MIDI uses configured output channel routing. Preview and saving do not apply the draft.

Saved drafts retain prompt, validated intent, edited note identities and regeneration keep
choices in an atomic versioned phone-local catalog (128 entries, 16 MiB). Invalid saves retain
the previous catalog; invalid loads retain the file. Drafts are outside portable session
archives. Optional tempo=40..240, progression=classic|pop|minor-turnaround and
rhythm=straight|offbeat|swing preserve the six-field legacy defaults. UI controls override
these optional prompt fields; tempo controls draft audition and does not change the accepted
sequence tempo. Regeneration retains resolved controls. Free-form inference is future work.

The combined APK builds with zero warnings/errors and all 504 composition checks pass,
including exact edited-draft persistence, bounded groove generation, malformed-file/failure
recovery and real-worker preview note cleanup. Storage rejects null nested structures explicitly.
No phone was connected for new audition/device interaction checks in this follow-up.

## Portable composition drafts — October 8, 2026

`CompositionDraftArchive` exports one named draft as a `.sloopdraft` ZIP containing exactly
`draft.json`. Schema 1 carries saved name/identity, original prompt, resolved musical intent
including seed/tempo/progression/rhythm, generator identity (`offline-composition/1`),
keep-part choices and the actual editable pattern: PPQ, loop length, pattern/revision IDs,
every note's ID/part ID/channel/pitch/velocity/onset/duration. Import does not regenerate notes.
Legacy local catalogs default missing generator identity to the existing procedural engine;
portable archives require every field explicitly. Other archive/generator versions reject.

Both compressed input and expanded JSON are capped at 4 MiB, including actual streamed bytes
from non-seekable providers. Import rejects malformed/truncated data, duplicate or unknown
JSON fields at any level, missing fields, bad entry CRC, invalid enums/numeric bounds/identities, unsafe or
unexpected ZIP paths, duplicate/additional entries, and symlinks. No archive path is extracted.
Validated adoption assigns a fresh catalog ID and uses the existing flushed temporary file
and atomic catalog replacement; repeat imports preserve existing entries and catalog-capacity
or publication failures preserve previous drafts. The archive remains outside session archives.

Composition review exposes **Export portable draft**; saved drafts offer export under **More**
and **Import portable draft** even when the catalog is empty. Android SAF selects source and
destination. Input validation/copying runs on a worker. Import shows name, prompt, generator,
resolved controls/seed, part/keep choices, note count and PPQ before **Import and open draft**.
Cancel performs no adoption. Confirmation saves a new local copy and opens the existing
editable-note review; applying to the current sequence still requires the separate full diff
and **Apply locally**. Import checks the captured workspace root and exact active source
pattern before reading, after worker validation and again before confirmed publication;
workspace/sequence changes reject adoption and ask the user to restart import. Subsequent
pattern application also retains the normal full-snapshot stale-proposal protection.
Picker state is activity-local; after activity recreation the user must choose import/export
again. Export provider failures may leave a partial destination document; local drafts remain
intact. Provider-side atomic publication is not claimed.

Integrated SAF dispatch: the beginning of shared `FileResultAsync` calls
`if (await TryHandleCompositionFileResultAsync(requestCode, resultCode, data)) return;`.
The composition partial owns request codes 7411/7412 and all UI actions; it does not modify
the shared dispatcher. Domain/storage focused tests cover real-format roundtrip, malformed,
unsafe, oversize, missing/duplicate/unexpected fields and entries, invalid note bounds, and
atomic preservation/capacity behavior. Android compilation and physical SAF review/cancel,
provider failure and activity-recreation verification belong to combined integration.

## Implemented vocabulary and workflow

Example: `key=C scale=minor bars=4 seed=42 parts=bass,chords,melody,drums density=steady`.
All six fields are required exactly once, separated by whitespace; order and letter case
do not matter. Parts are a nonempty comma-separated unique subset. Extra words, fields,
unknown values, overflow and duplicate parts reject with explicit errors.

- Keys: C, C#/Db, D, D#/Eb, E, F, F#/Gb, G, G#/Ab, A, A#/Bb, B.
- Scales: major, minor, dorian, mixolydian, harmonic-minor, major-pentatonic,
  minor-pentatonic. Minor means natural minor.
- Bars: 1..16 in 4/4; seed: 0..2147483647; density: sparse, steady, busy.
- Parts: bass (MIDI channel 1), chords (2), melody (3), drums (10).

In the existing app-pattern prompt command dialog, **Compose a new offline loop** opens
the vocabulary and example. Generation and regeneration run on a worker task. The review
shows resolved key/scale/bars/density/seed and every note in a selector. Each draft note can
be edited (pitch, velocity, onset, duration) or deleted, with normal app-note validation.
Keep checkboxes preserve chosen parts, including edits/deletions, during regeneration.
Regenerate increments the seed and replaces only unkept parts. Maximum seed rejects further
regeneration; a new lower-seed prompt can be entered. Keep choices persist across note edits
and successive regeneration reviews.

**Review apply** shows the existing full note diff, then **Apply locally** commits exactly
one `EditProposal` through existing `EditingWorkspace.EditPattern` history/storage. Applying
replaces all existing notes on requested part channels, retains other channels exactly,
and sets the loop length to the requested bars. Retained notes beyond a shorter loop cause
rejection; they are never truncated. Captured snapshots enforce stale-proposal rejection.
Composition apply and existing prompt edits use the same activity-local `EditLocks` as
the piano roll. Protected parts/notes reject complete conflicting proposals. Regenerated
draft prompts record the incremented seed so provenance agrees with the resolved intent.
The previous offline app/native recipe commands remain available.
The final diff identifies MIDI channels and explicitly shows old/new loop length when it
changes. Failed draft edits or regeneration preserve the current draft and keep selections
for another review; draft creation failures report the unsupported/invalid input.

The generator uses `AppPattern`/`AppNote` and `PerformanceHarmony`, a selectable classic, pop or minor-turnaround
progression, diatonic triads with existing voice leading, root bass, scale-grid melody,
and GM-style kick/snare/closed hats (36/38/42). Density changes subdivisions. A specified
32-bit PRNG and SHA-256-derived note IDs avoid runtime-dependent randomness; repeated
resolved intent produces identical notes and identities. Part streams are independent,
so adding another part does not change an existing part's notes. Pentatonic melody uses
the requested pentatonic grid; chords/bass follow the existing major/natural-minor harmony
mapping for pentatonic scales.

Native hardware draft conversion, model downloads, generated audio, sound selection,
portable session draft persistence and scene prompting remain future work. Draft MIDI audition,
tempo/rhythm/progression controls and device-local saved drafts are implemented. MIDI
channel 10 drum compatibility depends on the destination. Keep original prompt, resolved
intent and editable pattern in `CompositionDraft`; accepted history stores original prompt,
actual regeneration seed and rule version. Part-generation provenance across repeated keep
operations is not individually recorded. Cancellation tokens are supported by domain generation
and regeneration; there is no Android cancellation control yet. Timing resolution must be
divisible by four within 4..96000 ticks per quarter. Length-only changes with no note diff
reject because the existing history ignores empty note proposals.

Focused validation: `dotnet build android/src/Sloop.Composition.Tests/Sloop.Composition.Tests.csproj
-m:1 -p:RestoreDisableParallel=true`, then `dotnet run --project
android/src/Sloop.Composition.Tests/Sloop.Composition.Tests.csproj --no-build --no-restore`.
Checks cover parser rejection, identity/note determinism, musical bounds across scales/keys,
editing/deletion, keep/regenerate, cancellation, protected data, retained channels, stale apply,
one-step undo/redo and existing prompt recipe availability.

## Future experience beyond this slice

Editing existing material through prompts is specified separately in
[PROMPT-EDITING.md](PROMPT-EDITING.md), sharing musical intents and validators with creation.

Enter a musical description, preview a generated loop, keep or regenerate individual parts,
then edit the result with the same sequencer tools as a manually written pattern. Prompts
can refine the current loop: simpler bass, stronger swing, sparse hats, warmer chords.
Generated content is a draft. Applying a draft is undoable and does not overwrite a device
pattern until the normal pattern-send operation is invoked.

## Broader architecture

Prompt -> musical intent -> deterministic composition engine -> validated loop draft.
Musical intent records tempo, bars, scale/key, groove, density, harmony, roles, engine
preferences and a seed. A C# engine handles rhythm templates, harmonic constraints,
voice leading, register, velocities and variation. Hardware targets obey current track,
step, polyphony and parameter limits. Longer material uses the app arrangement layer.

Start with explicit controls and a limited offline phrase vocabulary. Label this procedural
composition rather than claiming a trained AI model is running. Later add a replaceable
local text-model adapter to translate arbitrary prompts into the same bounded intent.
Reject malformed or out-of-range model output; never execute generated code or arbitrary
protocol commands. Composition and inference run off the audio/timing threads and are
cancellable. Keep prompt, resolved intent, seed, model identity and generated result together.

## Offline model integration

Use an optional model pack installed once by download or local import, then operate without
network access. Validate its identity, compatible runtime, license and storage requirements.
Expose unavailable-model and insufficient-memory states, and retain the procedural engine.
Benchmark prompt fidelity, load time, memory, cancellation and thermal behavior on the actual
phone before selecting a model. Do not assume every supported Android device can run it.

LiteRT-LM was investigated as a possible Android inference adapter in the sources below;
selection requires fresh runtime/phone evaluation. Its adapter would keep workstation and composition logic in C#.
No runtime or model pack is selected or implemented yet. Future iOS/Windows frontends can
supply a different inference adapter behind the same musical-intent contract.

## Scope and dependencies

Output is editable symbolic composition and synth settings. AI-generated audio is excluded
from scope. Prompt-driven FM6 patch creation is scoped in [AI-SOUND.md](AI-SOUND.md) and can
ship independently of loop composition; it does not require a sequencer to generate a patch.
An LLM interpreting a prompt is not evidence that its generated notes are musically good.
Validate musical results by audition and structural constraints, with variation and
part-lock controls. Deliver after pattern editing, note ownership, preview/playback and
transactional pattern sending are available.

Sources checked 2026-10-07:

- [LiteRT-LM](https://github.com/google-ai-edge/LiteRT-LM)
- [Android LLM inference migration guidance](https://developers.google.com/edge/mediapipe/solutions/genai/llm_inference/android)
- [MuseCoco text-to-attribute-to-symbolic-music research](https://www.microsoft.com/en-us/research/publication/musecoco-generating-symbolic-music-from-text/)
