# NOTES piano-roll and hit-action integration proposal

Implemented and integrated: STEP zoom, NOTES, NOTE ACTIONS and HIT ACTIONS. The production target fits; hardware testing remains with the user.

## Implementation status

STEP and NOTES use PRESETS for 8/16/32/64-step zoom, starting at 16. The viewport follows selection independently of the physical keyboard bank, shows range/zoom, and joins independent sustains across mixed attacks and viewport boundaries. Zoom is transient and creates no undo entry.

The native synth page order is STEP → NOTES → NOTE ACTIONS → PATTERN → RECORD → SEQUENCES → SONG. NOTES knob 1 walks attacks and empty cells; knobs 2/3/4 edit the selected chain's pitch, length and packed strength. Pitch edits require stopped transport and preflight every affected cell. Selection is onset/pitch, resolved after slot compaction, and invalidated on track/project/context changes. NOTES and NOTE ACTIONS preserve selection between each other. EDIT taps on NOTES erase only that chain, with undo; held EDIT retains its native whole-pattern tools. Shared step accent remains shared, and FORCE displays STEP ACC when it overrides packed strength.

NOTE ACTIONS uses knob 1 for the row and knob 2 for a shared field or staged destination. Rows are SLIDE STEP, NUDGE STEP, FILL STEP, ACCENT STEP, MOVE NOTE, COPY NOTE and DELETE NOTE. OCT+ applies a discrete action once per press; OCT− returns to NOTES. Shared fields update from knob 2 and are explicitly labeled STEP. Physical keys are silent while the sheet owns input; function layers retain priority. PLAY/REC close the sheet and keep their transport meanings. Action edits require stopped playback/recording. Move/copy preserve other voices and refuse collisions, capacity, out-of-pattern ends and source-onset locks without modifying bytes. A selected move may overlap its own sustain. Destination shared timing/automation remains at its steps.

Drums retain GRID → KIT → SOUND → GROOVE → RHYTHM, then add HIT ACTIONS. Lane and source step are pinned on entry. Ratchet belongs to the selected hit; NUDGE/FILL are shared STEP fields. MOVE/COPY/DELETE carry only that hit's level/ratchet and preserve neighboring lanes and shared metadata. OCT− returns to GRID.

Implementation lives in ui_note_select.c, note_chain_actions.c, drum_hit_actions.c and ui_note_actions.c, with the existing native edit/undo helpers. Tests cover actual input/rendering and immutable zoom, 300 randomized selection patterns, 1,200 randomized chain patterns, overlapping moves, balanced MIDI release, drum lane/metadata preservation, staged actions, exits and layer ownership. The current production target is 580,708 bytes with 856 bytes of flash free; static main RAM is 89,064/98,304 bytes. All eleven focused editor/UI/MIDI suites, startup byte oracles, USB resampling, synth drums, target DSP budgets and 182 golden audio renders pass. No hardware flash is automatic. Upstream POLY recording/bit-7 legato remains a separate compatibility project, not part of this editor integration.

## Recommendation and evidence

Adapt the upstream visual ideas and compact action lists, not its input dispatcher or whole-step mutation helpers. Keep STEP as the default native editor; add an explicitly selected NOTES view and action pages. Independent continuations, existing chord merges, NEON presentation and native libraries remain authoritative.

Reviewed the actual `4ec1ac829fd2bdf2c571ea215fb3d62477d01220..b77bf55e3bf804665b8f5dffb5955bd8443311f0` source diff, including ui.c, ui_input.c, ui_draw.c, ui_studio.c, ui_layers.c, ui_menu.c, seq.c and core.h, against the working MerthSloop sources at base `1ce6a89`. The implementation is in upstream `beab3f62`; `b77bf55e` changes release documentation only. See [candidate assessment](UPSTREAM-CANDIDATES.md), [native editing](SEQUENCER-EDITING.md) and [independent ties](PER-NOTE-TIES-DESIGN.md).

Upstream adds three byte fields (`note_k`, `zoom`, `sheet`), 8/16/32/64-step windows, chord-slot walking, pitch/length/force controls, a 400 ms OCT+ sheet trigger and drum keyboard remapping. Its `note_walk` skips ST_TIE cells but does not resolve independent incoming continuations into unique attack events. `graph_roll` calls whole-step `note_len` for every chord member. `note_del` compacts the local chord fields; it does not perform our selected pitch-chain erasure. `note_shift` copies/moves the entire step and following ST_TIE run, rejecting occupied destinations. Thus an individual-note highlight does not imply individual-note move/copy semantics upstream.

Drum `drum_hit_shift` moves one lane's level/ratchet only; nudge/fill remain step-owned, and locks do not follow the hit. Upstream FORCE maps four packed per-slot levels plus the shared step accent; changing one member can clear the accent for its entire chord. These distinctions must be visible in our design.

## Explicit control mapping

Use page navigation instead of a timed OCT+ overload. Proposed synth page order: STEP → NOTES → NOTE ACTIONS → PATTERN → RECORD → SEQUENCES → SONG. SELECT walks pages as today; held function layers retain their current priority and SELECT tempo behavior. NOTES and NOTE ACTIONS are local UI pages, not new wire parameter IDs. Page transitions retain the cursor/pitch selection within this pair and clear it on track/load/context changes. STEP remains unchanged.

| Context / gesture | Proposed result |
| --- | --- |
| STEP, existing knobs and taps | All current behavior, including whole-chord pitch edits, selected-source LEN and existing EDIT deletion semantics |
| NOTES knob 1 | Walk attack events and empty cells in step order; chord attacks in stored slot order. Skip continuation-only occurrences of the same chain. Reverse walking is the exact inverse. Clamp at pattern ends; do not wrap a destructive selection |
| NOTES knob 2 | Selected chain pitch in semitones through atomic chain-aware preflight; stopped transport required |
| NOTES knob 3 | Selected chain length through native `note_set_voice_len` / `note_extend` preflight; never whole-chord `note_set_len` |
| NOTES knob 4 | Selected attack's packed level: GHOST, SOFT, NORM, HARD. ACC is a separate shared-step action, not a fifth per-note level |
| NOTES PRESETS encoder | Zoom 8/16/32/64, default 16, clamped; no audition, sound load or browser action |
| STEP or NOTES, OCT+ + knob 1 | Existing anchored tie brush; current ALL/voice selection survives crossings |
| STEP or NOTES, OCT− + knob 1 | Existing destructive rest brush, whole-step scope; no silent conversion into per-note erase |
| STEP or NOTES, either OCT held + knob 2 | Existing whole-event octave edit and its safety guards |
| STEP or NOTES, either OCT held + knob 3 | Existing whole-event move/merge and its stopped-transport/linked-pattern guards |
| STEP or NOTES, OCT+ + knob 4 | ALL/source voice selection exactly as today; labels show pitch and source |
| NOTES EDIT tap / hold | Tap erases the selected chain with undo; held whole-pattern tools retain their native meaning. DELETE NOTE is also available in NOTE ACTIONS |
| NOTE ACTIONS knob 1 / knob 2 | Select action row / change row value or staged destination. Knobs 3/4 inactive with labels blank |
| NOTE ACTIONS OCT+ tap / OCT− tap | Apply selected discrete action / return to NOTES. Neither starts a brush, octave change or delayed second action |
| Any action page, HOME / page or track change | Close page, discard staged destination, consume its entry/release edges; no project mutation |
| Any context, held EDIT / SEQ / other layer | Existing layer controls win, including undo/redo, step pages, nudge/fill and whole-pattern tools |

NOTE ACTIONS entry is SELECT navigation, never OCT+ duration. Its modal ownership applies only with no function layer held. No delayed OCT events may leak when entering/exiting it. PLAY/REC keep transport/recording meaning; recording/arming/free-take entry closes the action page and structural edits require stopped transport. Apply button taps cannot simultaneously act on the underlying grid. Hold does not auto-repeat COPY or DELETE; require a new released press for another execution.

Rows: SLIDE [STEP], NUDGE [STEP], FILL [STEP], ACCENT [STEP], MOVE NOTE, COPY NOTE, DELETE NOTE. A scrolling list replaces upstream's fixed six rows. Values for ordinary step fields may update per detent in one undo session; MOVE/COPY destinations are staged and OCT+ commits. SLIDE is the existing outgoing step flag, not SF_LEG. Shared fields always show STEP and affect all pitches/hits at that index. The selected target header shows step, pitch, voice and duration. Never claim per-note microtiming or per-note accent.

For drums, keep GRID → KIT → SOUND → GROOVE → SHAPE in existing order; append HIT ACTIONS, reachable by SELECT or the existing page cycle. Return with OCT− to GRID. Rows: RATCHET [HIT], NUDGE [STEP], FILL [STEP], MOVE HIT, COPY HIT, DELETE HIT. Header pins lane plus cursor on entry. Knob 1 chooses row, knob 2 value/destination, OCT+ applies. Disable grid entry and keyboard audition while the page owns input; function-layer performance remains available through the normal dispatcher. Preserve GROOVE/SHAPE OCT− listen/stop and OCT+ apply/confirmation/hold behavior by scoping all action-page handling to HIT ACTIONS.

### Drum keyboard recommendation

Do not take upstream's default black-key lane selection plus OCT page remap. GRID currently uses white keys for steps and black keys for banks; drum OCT keys also participate in ghost/hard dynamics and held SEQ page/fill/reset gestures. Retain these defaults and knob 1 lane selection, and keep SOUND's playable keyboard selection. Upstream's paired silent black keys (11 keys address 16 lanes by repeated presses) are less discoverable and would remove direct bank access.

If later requested, offer an explicit GRID-only SELECT keyboard mode, showing SELECT in the header: black keys silently choose/toggle the documented pairs; knob 2 still selects the absolute step/bank, avoiding any OCT page reassignment. It must not change SEQ layer or SOUND mappings. This option is deferred, not a prerequisite for hit actions. Optional preview and entry auto-advance preferences can land separately with settings persistence tests; do not copy upstream MIDI-channel byte packing without reconciling our MRS1 settings extension.

## Event model and mutation contracts

Use the existing ten-byte step layout and bounded native helpers in `note_ties.c`, `ui_input.c` and `ui_step_move.c`. An editor selection is transient `(track, onset, pitch)` plus ALL where appropriate; resolve a slot again after compaction, never retain an unchecked slot index. `note_effective`, `note_origin` and `note_chain_len` provide the basis for resolving legacy ties and independent chains. The initial UI resolver is linear from pattern start and explicitly does not promise editing a chain through the loop boundary.

Render each attack once and join only matching incoming continuation pitches; a deliberate same-pitch reattack starts a new bar. Effective ST_TIE content can cross a visible bank boundary. Ignore stale REST/TIE payload when calculating range. Auto-follow the selected pitch vertically, keep cursor window stable during playback, clip bars starting before the window, and distinguish playhead from selection. Zoom changes only the viewport, never step DIV/LEN or sequence content. At 64 steps, enforce minimum one-pixel strokes and avoid negative hollow-bar widths. Empty patterns still show an octave around last-note and an insertion cursor.

Keep NEON fonts, palette roles, framed headers, compact help and cached drawing. Reuse the audited current roll's four-slot hashes and continuation display, extending its viewport rather than replacing ui_draw.c wholesale. Cache keys include zoom, selected pitch/onset, all pitch/level/ratchet/flag fields, page/sheet, shared metadata and playhead. No enlarged knob overlay may obscure high voices. Force level affects brightness/fill, accent gets its own shared marker, and a compact numeric velocity remains available: packed level is not a lossless 0–127 velocity editor. Continuations display the attack's dynamics because playback preserves attack velocity.

Selected-chain pitch/delete/length must preserve unrelated attacks, packed slot levels/ratchets and continuation alignment. Pitch changes preflight the full chain for MIDI bounds, same-pitch reattacks and four-slot capacity; do not silently collapse two distinct attacks. Until this helper exists, retain current linked-pattern pitch/octave/move refusal. Length/delete may use the already implemented selected-voice operations after setting a validated source selection; deletion removes that chain and leaves other voices sounding.

Whole-event OCT move remains the current behavior: clean-rest collision checks, forward-into-own-ties shortening, merge into an existing chord, duplicate deduplication, four-note ceiling, destination metadata authority, compatible tie lengths and refusal for source locks on a merge. Never substitute upstream `note_shift` and lose this behavior.

New MOVE NOTE / COPY NOTE are a later, separately tested operation, not aliases to whole-event move. Preflight source and destination extents before mutation; merge with unrelated voices across every affected cell, reject capacity and deliberate same-pitch reattacks, preserve destination step metadata. Carry per-note level/ratchet and rebuild its continuation bits; COPY leaves the original intact. Single-note operations do not migrate shared micro/fill/slide/locks: retain them at their steps and label this policy. Refuse ambiguous source-step lock automation with SOURCE HAS LOCKS until a reviewed policy exists. Keep whole-event moves as the route for moving shared onset metadata. No loop wrapping. A failed operation changes neither pattern, cursor nor undo state.

Drum move/copy merges into steps containing other lanes but rejects an already occupied destination lane. Transfer only the selected lane's level/ratchet; retain destination step nudge/fill/locks, preserve source shared metadata, and warn via STEP labels. Do not clear other lanes. This matches the data model rather than implying hit-specific automation. Every successful gesture shares one existing undo session; use the starter/groove metadata supplement for changes it covers, never allocate another pattern snapshot. Capture undo only after preflight; test byte-exact restoration of steps, micro, fill and locks. Do not supersede project-load recovery by simply opening a view.

## Compatibility gates and independent delivery

| Work | Dependency / delivery boundary |
| --- | --- |
| Zoom and roll, action navigation, selected-chain delete/length, four-level display, existing step/hit metadata edits | Existing continuation bits 3–6 suffice. No SF_LEG, recording rewrite, FUN6 revision or Android UI work required |
| Selected-chain pitch/move/copy | New bounded native mutation helpers and collision/undo tests; no bit-7 dependency |
| Improved POLY held recording | Can be adapted independently using existing continuation representation, but must preserve fork snap, ownership, overdub and arp behavior |
| MONO/LEGATO/UNISON overlap recording and upstream legato playback | SF_LEG compatibility stage required before enabling creation or claiming end-to-end support |
| Silent lane keyboard mode, preview, auto-advance | Independent preferences, reviewed gesture/settings policy; not required for action sheets |

`SF_LEG = 0x80` is an incoming monophonic legato transition, distinct from continuation slots and outgoing SF_SLIDE. Upstream uses it for gate lookahead and envelope continuity; it alone must not request a glide. Keep bits 3–6 and reserved legacy bit 2 unchanged. Do not fix transport by shifting continuation 4 into bit 7.

FUN6 keeps its existing binary size; raw project bytes can preserve bit 7, while older firmware may reattack it. That is structural compatibility with a musical downgrade, not full semantic compatibility. Audit capture/apply, normalization, project save/load recovery, preset reductions and file conversion separately. A raw backup round trip does not validate STEP_SET or preview behavior.

Concrete current protocol blocker: `editor.c:ed_b` masks every byte with 0x7f; `ed_step_in` permits only accent/slide/continuations. Its existing v5 extension carries lvl high in extension byte bit 0 and rat high in bit 1. `web/editor.html` request builder and local simulated STEP_SET mask flags to 0x7B. Android `Sloop.Sequencing/Models.cs` lacks Legato; `Validation.cs` rejects bits outside 0x7b. Upstream's three-line web diff is not a coordinated solution.

Recommended wire design for a future legato stage: negotiate a new editor protocol capability and use bit 2 of the existing dynamics high-bit extension byte for flags bit 7, keeping all transmitted bytes seven-bit clean and old record lengths intact. This bit allocation is a proposal, subject to a full protocol audit before assignment. Upgrade response/notification encoders and request decoders symmetrically; legacy peers keep their current layout. A capable editor must warn/refuse destructive editing of legato data against incapable firmware rather than silently clearing it. Do not reinterpret legacy flags bit 2. Preserve opaque binary backups byte-for-byte.

Update Android `Sloop.Protocol/EditorCodec.cs`, sequencing enums/validation and hardware-step conversion together; test file codecs in Fm1Songs and MIDI-file conversion as well as live transport. Add semantic preview playback in native preview/WASM and web simulation; preserve flags through JSON import/export, roll conversion, slot sorting, move/delete and MIDI import. Browser `RV`/event conversion currently needs a continuation-aware audit even without legato: retaining flags is not proof it preserves independent durations. Legacy user-preset 16-step mono reductions need explicit lossy-conversion feedback. Do not regenerate packaged web/editor copies or WASM until the separately authorized implementation is validated.

Canonical fixtures should include old 2.7.1, real upstream 2.8 legato data and a fork mixed four-voice continuation pattern. Cover 0x80, 0x78 and combined meaningful flags, high level/ratchet bits, inactive slots and unknown peer capability. Verify binary bytes, editor messages and audible/MIDI behavior separately.

## Resource estimate and release gates

No build, prototype, package generation or generated-header write was performed for this review. Target costs below are planning estimates, not compiler measurements. Upstream's cumulative binary/WASM size delta includes effects and sample changes and cannot isolate NOTES. Its sample generator drops SCRCH backspin/rewind to recover roughly 20 KB; do not import that content loss as an incidental budget fix.

Source-sized planning allowance: incremental roll zoom/selection/display about 1–3 KiB flash; shared action renderer/router plus synth/drum actions about 2–5 KiB; fully validated selected-chain structural edits about 1–3 KiB more. Combined 4–11 KiB is deliberately broad and can change with inlining, deduplication and replacement of existing drawing code. It exceeds the documented current reverb build's 1,452-byte headroom (580,112-byte image); that document describes concurrent work, not a fresh measurement here. Earlier tie builds had only tens of bytes free. Full integration requires measured savings or a smaller scope before release.

Transient UI state should remain under a planned 64 bytes (selection, zoom, page/row, destination and cache words), plus a few ten-byte local step temporaries; no track/project growth, pool allocation, second framebuffer or pattern buffer. Upstream's three bytes may incur struct alignment, and octp timer/state adds storage if copied; this design avoids that timer. Text/pointer tables consume read-only flash on target; verify placement and stack high-water rather than counting them as free. Bounded scans cover at most 64 cells × 4 slots; avoid nested rescans in render loops where a forward pass suffices. Mutation preflight should run outside IRQ masking, then commit in a short bounded critical section with source/revision revalidation.

For implementation, compare target map/image/static RAM/pool/stack results against an identical baseline containing the accepted reverb and low-conflict integrations. Host object sizes are only directional and cannot prove target fit or audio deadlines. If a host prototype is useful, scope it to a separate scratch directory with its own outputs and no invocation of repository generators/build scripts that touch shared headers. Physical encoder/display latency and audio timing remain hardware gates.

## Implementation sequence and tests

1. **First small milestone:** adapt current roll to 8/16/32/64 zoom using PRESETS on STEP only (before adding NOTES/actions), with unchanged editing controls and source selection. No recording/protocol/structural mutation changes. Add viewport/clipping/cache checks and measure target delta against the settled baseline. Acceptance: unequal chord durations remain correctly joined, bank-crossing ties are visible, all held gestures and libraries behave identically, target fits. This can be reviewed independently of the complete NOTES design.
2. Add NOTES selection resolver and navigation with safe guards; selected-source LEN/delete and level display. Keep pitch/structural guards until chain helpers pass. Add explicit NOTE ACTIONS/HIT ACTIONS navigation and shared-field labels; retain all existing browsers and settings.
3. Add chain-aware pitch, then move/copy as distinct reviewable changes. Only enable supported rows; disabled rows explain the limitation instead of invoking whole-step upstream helpers.
4. Adapt POLY recording separately. Coordinate bit-7 protocol, files, preview and Android before enabling SF_LEG recording/playback. Publish artifacts only after a separately authorized release.

Relevant existing regression anchors:

- `tests/note_ties_ui_test.c`, `seq_note_ties_test.c`, `note_trails_test.c`, `preview_note_ties_test.c`: source retention, unequal durations, third/fourth-slot redraws, legacy ties, attack identity, gate expiry, balanced MIDI and preview parity.
- `tests/seq_navigation_test.c`, `edit_tools_test.c`, `ui_pages_test.c`, `ui_refresh_test.c`: existing move/merge, whole-pattern shift/length/transpose/octave, bounds, layer priority, forced/incremental framebuffer equivalence and UI fuzz. Adapt upstream ui_pages cases as scenarios, not expected fork mappings.
- `tests/studio_drums_test.c`, `drum_grooves_test.c`, `editor_drum_grooves_test.c`, `sequence_starters_ui_test.c`, `sequence_starters_test.c`: lane/cursor identity, grid keyboard banks, groove/SHAPE audition/apply confirmation, sequence browser audition/apply, metadata undo.
- `tests/project_test.c`, `upstream_project_test.c`, `section_save_test.c`, `song_ui_test.c`: raw fixtures, durable save/load, section changes, undo-load/redo and failure recovery. New page visits must not create musical undo entries or disturb recovery availability.
- `tests/seq2_test.c`, sequencer/arp regressions and upstream 2.8 recording scenarios: snap, independently released overlapping notes, same-pitch reattack, full chord, mono overlap, STOP/panic/mute/scene/load/track cleanup, sequence-fed arp and SNOTE ownership.
- `web/test_web.mjs`, `tests/preview_test.mjs`, Android Sequencing/Transport/Fm1Songs/MidiFile/Integration test projects and `android/scripts/Test-Domain.ps1`: codec symmetry, old-peer negotiation, import/export and preview behavior.

Add explicit input traces for OCT+ held beyond 400 ms without opening anything, each held knob combination, all action-page exits/releases, layer undo from actions, repeated apply presses, record-arm arrival, zoom while held, empty/short patterns and extreme pitches. Structural tests must assert unchanged bytes on every rejection, complete undo/redo and neighboring-track isolation. Hardware tests include fast/reverse encoder turns, live playhead rendering and held help readability in NEON.

Known limits remain four simultaneous sequenced pitches, shared step accent/velocity/micro/fill/locks, legacy whole-step slide, initial no-wrap chain editing, current guarded linked-pattern pitch/move, and incomplete web/phone duration authoring and held recording. Existing SNOTE/arp whole-chord harmony publication is not upgraded by a visual roll. None of these may be advertised as resolved by merging upstream UI code.

The original design review was source-only. Current integration and regression status is recorded above; measured final target size is tracked in PIANO-ROLL-SIZE.md and OPTIMIZATION.md.

## Piano-roll display refinement

STEP and NOTES now draw shaded black-key rows, labeled C octaves, four-step beat
lines and stronger bar boundaries. A white frame marks the selected independent
sustain; the playhead crosses the graph. Wide pitch ranges follow the selected
note. All drawing stays in the graph below the four NEON cards. PRESETS turns
left to zoom in (8 steps), right to zoom out (up to 64), without changing sound
or editing the pattern. Independent chains remain continuous across other onsets.
