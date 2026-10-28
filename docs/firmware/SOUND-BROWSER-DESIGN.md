# On-device sound explorer

SAVE → SOUNDS extends the existing native preset list. It retains its stable
factory identities, favourite order and occupied user-slot mapping. MIDI Program
Change and HOME/PRESETS browsing keep the original unfiltered indexing.

## Controls

Knob 1 and PRESETS audition within the list. Knob 2 chooses ALL, FAV, the existing
11 sound kinds, USER, then each factory engine. Engine filters exclude user
presets; USER collects those slots regardless of engine. Empty lists do not load
a sound. Knob 3 right accepts the candidate; left restores the original and resets
the filter to ALL. Knob 4 adds/removes favourites independently. The selected row
shows its engine in the selected track's color. Normal notes/transport work.

## Audition ownership

The first candidate captures one track's base parameters (behind active locks),
engine/preset/user identity, favourite cursor, A/B captures/position, and actual
FM6 operator payload/slot/pending state. Further candidates retain that original.
The record contains no voice pointers, sequencer memory or persistent storage.
Normal preset loading still preserves track effects and playing preferences.

KEEP clears the transaction. A later candidate captures a new baseline. Cancel,
page changes, held-function presses and track changes restore the original patch;
normal sound-change fading applies. Audible envelopes/phases are not rolled back.
A project adoption discards the transaction before installing its new state.
MIDI Program Change cancels an audition before applying the requested change.
Finish an audition before editing the patch from an external editor.

Autosave refuses pending candidates. Keyboard recording and transport remain
available; notes are not included in the rollback. KEEP accepts a working sound,
not a new user preset or an explicit section save. A/B captures remain runtime-only.
On the drum track, the existing kit browser remains in use.

## Validation

sound_browser_test uses the actual UI and factory/user loading paths, checking
category/engine projections, empty lists, wrapping, favourites, track/page exits,
live knob input, external PC, all-engine cancellation, unsaved FM6 payloads,
A/B capture restoration and pattern/other-track preservation. headroom_commit_test
covers actual autosave refusal/resumption and project adoption. Native screenshots,
UI fuzz, audio goldens, core splits and timeout recovery supplement these checks.
See [literal storage](LITERAL-STORAGE.md) for the space funding and memory trade.
