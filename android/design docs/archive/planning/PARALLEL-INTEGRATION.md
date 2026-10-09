# Combined integration — 2026-10-07

> Reconciled October 9, 2026: The integration instructions below are historical. Current integration includes FM6 bank saving, phone musical starters, external MIDI input and expressive arpeggios. See [STATUS](../../STATUS.md) and [verification](../../VERIFICATION.md).

All four parallel slices are wired into the Android shell. The solution includes their new
projects and integration test runners; all earlier uncommitted work is preserved.

## Follow-up integration

Three additional chats delivered scene hardware reconciliation, stricter offline prompt
editing and sample-kit review. The shell now includes scene rename/duplicate/recapture/delete,
explicit arrangement repeats/reordering and cancellation of superseded preparation requests.
Scenes → Send scene sounds to FM1 verifies fixed Synth 1→1, 2→2, 3→3 baselines and performs
sequential stopped-only patch/macro transfer with complete per-track readback. Unknown results
block another transfer until a fresh Read. Pattern/samples/tempo remain separate.

Sequence prompt entry supports exact commands over a whole channel, selected bar or step,
with an optional hard lock on selected-step notes. Complete note changes are reviewed before
Apply through existing undo history; native selected-step commands also have reviewed entry.
Sample source/encoded audition, validated assignments, root ranges and proposed USR destination
are integrated. KitUpload consumes the proposal with all stale/backup/journal/readback guards.

Coverage now totals 5,041 checks/scenarios plus nine golden fixtures: eight new scene hardware
fault checks, 79 kit mapping checks, six catalog checks, three prompt sequencing scenarios and
five sound parsing assertions. Physical FM1 transfer remains unverified.

Follow-up Pixel validation: final APK installs and cold-launches in 766 ms. Prompt entry
shows selection/lock controls and full pitch/tick/length/velocity review; Discard retains
the original note. Scene management actions and arrangement add/repeat/reorder/remove menus
are reachable; test arrangement was cancelled. Sampling restores the retained WAV/chops,
keeps waveform actions visible, and displays selected source/encoded audition and verified
instrument assignment (chop 01, C4/MIDI 60, keys 0–127). No hardware send was attempted.

## Delivered

- Sequence continuously loops through connected MIDI. Perform captures actual emitted notes,
  supports silent count-in/overdub/channel replacement, saves an undoable pattern and releases
  owned notes on navigation/interruption. Visible offline recipe entry supports reviewed edits.
- Sampling supports zoom/pan/fit, precise shared-edge edits, transient preview/apply/discard,
  revision-aware kit invalidation and existing backup/readback upload flow. Split/Undo/Play/Stop
  remain directly below the waveform; optional navigation is an expandable section.
- Library saves named sessions and performance presets, imports/exports bounded hashed archives
  and loads sessions. Settings include performance options, sequence tempo and generic output/edit
  channels. Scene catalogs, preset assets and current sample original/sidecars travel with sessions.
- Session loading stages a writable generation, validates all editors/settings/sample/catalog
  before pointer publication, then adopts prepared references together. Old generations remain.
  Startup validates the full active generation and recovers a valid previous generation or retained
  legacy workspace when damaged. Failed preparation/stale commit preserves live files.
- Sequence → Scenes & arrangements captures complete app scenes, restores while stopped, queues
  strictly-next beat/bar/phrase transitions, creates ordered arrangements/repeated phrases and
  plays them through connected MIDI. Scene transitions preserve PPQ/epoch, release outgoing notes
  before incoming onsets and discard unsent outgoing events. Phrase origins handle mid-phrase
  switches. All unique arrangement scenes are prepared before playback; external edits cancel
  pending work. Boundary adoption runs on the UI owner while transport waits before new notes.
- MIDI CC sweeps use explicit channels, native 0–127 values and sixteenth-note evaluation with
  value deduplication. Outgoing automation stops on scene replacement/cancellation.
- FM6 visual/manual editing and visible Sound → Describe a sound are retained.

## Validation

| Runner | Passed |
| --- | ---: |
| Protocol/simulator/sampling | 410 |
| Sound design | 1,332 |
| FM6 editing | 2,891 |
| Sequencing | 20 |
| Sample encoding | 102 + nine Python golden fixtures |
| Workstation | 61 + 79 kit mapping |
| Transport/capture | 18 |
| Sampling tools | 32 |
| Sessions/archive | 32 |
| Workspace generations | 19 |
| Scenes | 26 |
| Scene hardware fault reconciliation | 8 |
| Scene/transport integration | 11 |

Total: 5,041 checks/scenarios plus nine encoding fixtures. Domain builds and combined Android
APK build pass with zero warnings/errors. No overlapping APK builds were used.

Pixel 7a: installed/cold launched; captured Integration-check-scene; saved Integration-checkpoint
with 14 assets; changed FM6 algorithm 5 → 6; session load restored 5; stopped scene restore
completed; restart restored the adopted sample (1.58 s, 48 kHz) and workspace. The verification
checkpoint and scene are retained as usable Library entries. Sample navigation UI is reachable.

Firmware 2.4.2 Merthsoft passes fresh target build, SAVE/New UI tests plus 20,000-frame fuzz,
USB playback variants and loader regression. The local installer checks the downloaded package
SHA-256 and device identity. See [RELEASE-2.4.2.md](../releases/RELEASE-2.4.2.md).

## Practical limits

Physical generic MIDI/FM1 loop timing, simultaneous touch, routing/USB duplex, scene switching
and sample-transfer sound acceptance remain unverified. Current scenes adopt local FM6 sounds;
they do not automatically write them to hardware. Explicit stopped-only FM6 scene sound
transfer is available with complete baseline/readback reconciliation. Native pattern references, hardware macro
automation and atomic hardware scene transactions remain future capability-aware adapters.
Current CC evaluation is stepped, not audio-rate. Queued sample/device/recording work rejects
scene adoption before pointer publication. Clock timing/jitter under heavy UI/storage load is
not measured. Prompt entry uses offline recipes, not conversational model inference.

No firmware was flashed, no changes committed, and unrelated services were left running.
