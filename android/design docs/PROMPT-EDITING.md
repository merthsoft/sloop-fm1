# Editing existing work through prompts

Current delivery: [STATUS.md](STATUS.md). Build and test evidence: [VERIFICATION.md](VERIFICATION.md).

2026-10-07 follow-up: exact offline `PromptEditor` commands are integrated into Sequence's
existing prompt entry. Select whole channel, selected bar or selected step; optionally lock
the notes at the selected step. Proposed edits show pitch, onset, duration and velocity
changes before Apply locally; unsupported clauses reject entirely. Native selected-step
commands have a separate reviewed local entry. Both use existing transactions and undo/redo.
No conversational inference is supplied. See VERIFICATION.md for exact grammar.
Current suites pass 20 sequencing scenarios and 1,332 sound assertions.

Status: shared design contract; FM6 and sequencing local proposal/transaction foundations implemented.
FM6 Android prompt controls, manual editing, local persistence and hardware adapters are
integrated. Exact offline pattern commands now return reviewable local proposals; the
Android review/apply panel is integrated. Prompted sample/scene/performance edits remain planned. 2026-10-07.

## Implementation status

The FM6 domain implementation lives in
[Sloop.SoundDesign](../src/Sloop.SoundDesign/README.md). `SoundIntent` and `Refinement`
provide bounded intent, `PhraseParser` recognizes a limited offline vocabulary, and
`ProceduralDesigner` generates/refines immutable `PatchDraft` proposals with hard locks,
field diffs, provenance and complete before/after patch/macro states. `RefineDraft`
supports preview follow-ups while preserving the original target revision and undo state.

`SoundDocument.Apply` validates target, revision, before-state equality and locks;
undo/redo restore complete local states without rerunning generation. Planning supports
cancellation and leaves the source untouched. Undo/redo history is session-local; Workstation
persists current sound state and accepted proposal provenance separately before adopting
the candidate. A unified cross-editor saved history remains planned. The 1,327-assertion
foundation suite covers FM6 data and local transaction behavior.

This establishes the FM6 portion of this contract, not the complete prompt-editing product.
Android prompt controls, four variations, locks/diffs/apply/discard and manual FM6 edits are
available. Unsupported negation rejects the request. Explicit RAM apply/readback and MIDI
audition of the current device sound are integrated; automatic hardware A/B Keep/Restore,
local FM6 rendering, inference and other prompted domains remain planned. Local undo does
not claim hardware restoration. See
[AI-SOUND.md](AI-SOUND.md#implementation-status) for the implemented controls and limits.

The sequencing portion now lives in [Sloop.Sequencing](../src/Sloop.Sequencing/README.md).
`AppEditor` and `HardwareEditor` accept bounded typed operations/selections, preserve stable
identities and unselected content, enforce hard part/event locks and representability, and
return immutable `EditProposal` states with `NoteChange` or complete `TrackChange` diffs and
`EditProvenance`. `EditHistory` requires the exact parent snapshot, groups compound gestures
and provides full local undo/redo without rerunning recipes. The package-free suite passes
20 scenarios covering musical preservation, invalid edits, exact offline commands and stale proposals.

App-note move, resize, transpose, duplicate, quantize, velocity and seeded thinning are
implemented. Hardware edits preserve native steps and parameter locks, support constrained
whole-step operations and selected drum-lane thinning, and reject unsupported independent
note duration and lane-only timing edits. See [SEQUENCING.md](SEQUENCING.md) for limits.
`PromptEditor` now supplies exact offline phrase commands for existing app notes and native
patterns. It captures the caller's explicit selection/locks and returns ordinary proposals;
it never applies content or sends hardware messages. App commands include numeric transpose,
move, resize, quantize and velocity changes plus seeded `simplify`. Native commands support
transpose, whole-step move, velocity, microtiming quantize, hit levels and drum thinning,
subject to the existing target constraints. Unsupported wording, compound requests, incorrect
units, negation and empty app selections reject the whole request. FM6 parsing also rejects
unknown clauses and conflicting directions/families instead of proposing partial edits.
See [verification record](VERIFICATION.md) for exact grammar and wiring.
There is no free-form model inference or persisted undo history. Reviewed pattern prompt
editing, procedural loop composition and manual scene editing are integrated. Android manual note editing, one-shot MIDI playback, persisted content and native
read/compare/write/readback are integrated separately through Workstation.

## Experience

Describe a change to the selected material: a pattern, chord sequence, sample, patch, scene
or performance layout. The app proposes a specific edit, previews it, and lets the musician
apply, refine or discard it. Manual editing and prompting operate on the same documents.
Keep the selected material and locked parts visible while prompting; never require users
to re-describe their project from scratch.

The first priority is useful revisions: simplify, intensify, change groove, transpose,
reharmonize, alter a timbre, reorganize chops or create a variation. New loop generation
and FM6 patch generation use related intent types but are separate Create operations.
AI-generated audio is excluded. Synth playback, recording and DSP analysis remain ordinary
audio functions.

## Concrete use cases

| Target | Prompt | Proposed result | Preserve by default |
| --- | --- | --- | --- |
| Drum pattern | Keep the kick, simplify the hats, add one fill at the end | Remove selected hat events; insert bounded final-bar fill | Kick, snare, tempo, bar length |
| Bass pattern | Make this more syncopated without changing the notes | Shift or redistribute selected onsets within the phrase | Pitches, key, phrase length |
| Whole loop | Make a quieter breakdown | Draft a new variation/scene with fewer events and lower selected levels | Original scene and its sound choices |
| Chords | Make this darker and smooth the chord changes | Suggest alternate chord qualities and closer voicings | Melody, key unless requested otherwise |
| Melody | Add a variation for the second half | Edit the selected half with bounded motif changes | First half, phrase identity, register |
| Sample | Chop on strong transients into at most eight pads | Candidate source-frame boundaries and pad order | Immutable source, current trim unless requested |
| Sample chops | Merge the short slices and leave the first hit alone | Remove selected boundaries | First slice, source and unaffected slices |
| FM6 patch | Make it darker but keep the attack and tuning | Lower selected modulation levels or change spectral decay | Locked envelope stages, pitch, algorithm unless needed |
| Sound macro | Make one knob move from soft to aggressive | Draft bounded destination curves for modulation/feedback | Original patch and unrelated mix settings |
| Scenes | Make an eight-bar intro, then bring in bass | Draft scene references, variation states and chain entries | Existing scenes and source patterns |
| Performance layout | Use D minor, bass on Synth 1 and chords on Synth 2 | Draft chord voicings, destination mapping and control layout | Track sounds, sequence data |

Promises depend on the target editor. If a target cannot represent the requested result,
explain the limit and propose an explicit alternative. For example, extended sample tails
can require overlapping source ranges, which the initial contiguous chop model does not
support. Do not claim to preserve tails while merely moving a boundary and cutting them.

## Entry points and context

Each editor has an Edit with prompt action beside its normal tools. It opens a compact
panel containing the target name, selection/range, locks, prompt, and suggested refinements.
Default scope is the visible selection; whole-track, whole-pattern and whole-session scope
are explicit choices. A selection-free prompt states which object will be edited.

Build context from the actual document, current engine/capabilities and selected revisions.
Include only what the requested edit needs: notes, rhythm, patch fields, source-frame markers,
scene references or controller bindings. Labels alone are insufficient for musical edits.
An active recording is not an editable completed take until it has been finalized.

Follow-up prompts refer to the current draft: less busy, keep the previous bass, halve the
variation, make the release shorter. Show whether refinement targets the draft or the
currently applied version. Switching selection must not silently retarget an existing draft.
Typed input is first; optional offline speech recognition can feed the same text boundary later.

## Processing pipeline

1. Capture a stable target snapshot, selection, capabilities, locks and revision identity.
2. Interpret the prompt into a bounded typed EditIntent. A local phrase parser is the baseline;
   an optional on-device language model provides richer interpretation later.
3. Resolve musical ambiguities into visible assumptions. Ask a short clarification only when
   choices materially change the intended target or cannot be resolved from context.
4. Run a deterministic domain editor against a copy: rhythm/harmony rules, sample DSP,
   FM6 recipes, scene planning or performance mapping. The model supplies intent, not code.
5. Validate the result, locks, target representability, dependencies and device limits.
6. Produce an EditProposal containing a before/after diff, audition data and complete undo data.
7. Apply to the local document only when chosen in the editor. Sending to hardware remains
   the corresponding normal device operation with acknowledgment and recovery handling.

Do not silently truncate a proposed arrangement to 64 steps, drop excess sample zones,
rewrite a locked part, or map an unknown parameter ID onto the wrong engine. Report the
constraint and offer a representable proposal. If inference fails, ordinary editing and
the limited offline phrase commands remain usable.

## Shared C# contracts

The following cross-editor concepts remain proposed public APIs. FM6 currently uses the
domain-specific contracts above. Sequencing has its own `EditProposal`, but not the complete
cross-editor contract below, including assumptions, audition and persistence:

- EditTarget: stable document/part identity, selection, content revision and capability identity.
- EditIntent: domain, operation, bounded attributes, preserve/lock set, seed and unresolved terms.
- EditProposal: parent revision, immutable before/after states, typed changes, assumptions,
  validation results, audition plan, generation identity and source provenance.
- ApplyEdit: verify parent revision, apply the complete local transaction, append undo history,
  and persist the new state. Discard does not change the source document.
- Domain editor: accepts snapshot plus intent, returns a proposal; cancellation leaves the
  source untouched. Shared code references no Android or inference runtime.

Use stable event/slice/operator IDs where identity matters. A pitch change and an onset shift
should remain edits to the same note where possible. A split/merge carries explicit identity
mapping. Undo restores all affected fields and dependent references, not just the visible
notes. Redo replays the accepted result without running inference again.

Saved history retains the prompt, resolved intent, seed, recipe/model version, parent and
accepted result. Reproducing an accepted edit uses stored content; identical seeds alone do
not guarantee identical output after a model, recipe or firmware update.

## Preview and applying changes

Show an understandable summary plus the actual changes: added/removed/moved notes, chord
voicings, marker positions, patch fields or chain entries. A/B auditions the original and
draft through the same phrase and output route. Pattern previews require a supported playback
backend; sample previews and synth-patch previews have separate dependencies.

Local drafts can be made without hardware. Hardware audition snapshots affected device
state, releases owned notes at transitions, verifies replies/readback, and offers Keep/Restore.
FM6 patches use the workflow in [AI-SOUND.md](AI-SOUND.md). Applying a local edit does not
implicitly write flash. Ordinary RAM edits, pattern sending, bank save and sample upload retain
their own device semantics and interruption handling.

The FM1 speaker/headphone output is the intended USB preview destination when available.
Use actual output-route discovery and verification; MIDI connection is not evidence of USB
audio playback support. See [AUDIO.md](AUDIO.md). The checked-out SLOOP firmware implements
the host-to-FM1 USB audio path for app-rendered playback; user hardware acceptance is complete for now. Sample playback integrates Android output
route checks; local synth rendering remains future work.

## Locks, conflicts and uncertainty

Locks are hard constraints enforced by the domain editor and validator. Make them visible:
keep kick, keep melody, keep pitch, keep attack, keep first slice, keep algorithm. Soft goals
such as warmer or less busy can be approximated, with the approximation stated.

If the parent document changes after proposal creation, mark the proposal stale. Rebase by
recomputing against the new snapshot or regenerate; do not overwrite newer manual edits.
For current firmware without sufficient revision notifications, re-read relevant hardware
state before applying/restoring and compare it with the snapshot. Exact arbitration of
simultaneous panel edits may require firmware revisions; do not present read/compare/write
as an atomic transaction when it is not.

Inference output must fit the intent schema and permitted operation set. Names, prompts,
metadata and model text are data, never executable scripts or wire commands. Invalid output
gets rejected or reinterpreted once through the constrained path, with a useful error on failure.
Cancellation during planning discards the proposal. Cancellation during device sending uses
that operation's existing recovery rules; the prompt engine must not invent successful undo.

## Offline implementation and priorities

Keep all editing engines and document transactions in C#. Android supplies local inference,
audio and storage adapters. Start with a small supported phrase vocabulary and visible
musical controls; label recipe/rule execution accurately. Later install an optional local
model pack once, by download or file import, then operate offline. No cloud fallback occurs
implicitly. Measure model load time, memory and intent fidelity on the Pixel 7a before choosing
the model/runtime. The interface remains compatible with future Windows/iOS adapters.

| Priority | Deliverable | Dependencies |
| --- | --- | --- |
| 1 | Sample edit commands: equal chops, marker merge, trim, undoable proposals | Existing sample editor; add transient analysis for transient prompts |
| 2 | FM6 refinement: darker/brighter, decay, velocity response, locked fields | Validated patch codec/editor and sound recipes |
| 3 | Pattern refinement: density, syncopation, fills, selected-part variation | Pattern documents, stable note IDs, scheduler/device sending |
| 4 | Harmony: voicing, reharmonization, motif variation | Chord/note models, constraints and audition |
| 5 | Scenes and performance-layout prompting | Existing scene transactions, mapping and note ownership |
| Optional | Free-form local model and personalized recipe ranking | Phone benchmark, model-pack lifecycle, domain-quality evaluation |

These priorities are a proposal; they do not turn unimplemented editors into available app
features. Prompt UI should expose only supported targets and operations in each shipped slice.

Priority 2's platform-neutral codec, recipes, phrase commands, locks and local proposal
transactions, manual editor, Android prompt flow, persistence and explicit device adapters
are implemented. Guarded reversible RAM A/B/Restore/Keep is integrated; device bank saving remains future work.

Priority 3 now has platform-neutral pattern models, stable identities, typed edits,
selected-part thinning, validation, exact offline phrase commands and local transactions.
Syncopation/fills and free-form sequencing interpretation remain future work; app
arrangement scheduling is integrated. Android manual
controls, basic MIDI preview and native device sending are integrated. PerformanceHarmony
now supplies manual chord voicing/voice leading; prompted reharmonization remains planned; bounded procedural composition is implemented.

## Acceptance

Test meaningful musical edits and preservation: kick remains identical under hat simplification;
melody stays intact during reharmonization; note pitches remain fixed under rhythmic change;
locked FM6 values do not change; chop edits keep the original PCM bytes untouched.
Validate full transaction undo/redo, edit persistence, stale-parent conflicts, canceled
inference and unavailable model/runtime behavior. Keep a small prompt corpus with explicit
expected constraints and audition judgments; valid JSON alone is not success.

Test out-of-range note times, overlapping/empty slice proposals, unsupported engines,
oversized patterns, excessive zones and unrepresentable scenes. Require useful refusal or
alternative results rather than silent loss. Check hardware apply/readback and disconnect
recovery for every supported device operation. Confirm recipe mode, and later installed-model
mode, work without internet; preview failures must not destroy the accepted musical content.
