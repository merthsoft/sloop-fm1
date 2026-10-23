# Native recording snap and STEP editing

For sustained notes across later attacks, use the
[independent per-note tie controls](PER-NOTE-TIES-DESIGN.md). OCT+ + knob 4 selects
ALL or one source pitch; OCT+ + knob 1 extends it while retaining other attacks.
Released LEN and EDIT shorten/delete the chosen voice. NOTES supports independent
voice pitch and length changes; NOTE ACTIONS moves, copies or deletes one selected
voice and its sustain. The legacy OCT-held whole-step move refuses independent
ties; use NOTE ACTIONS there. Whole-pattern transposition remains available.

Recording snap and tie/rest painting shipped in 2.5 Merthsoft.6. The controls below
reflect Merthsoft.7, which adds octave edits, moves event editing to knob 3 and allows
forward moves into an event's own ties. No wire parameter IDs or project formats change.

## Painting lengths and silence

Select a synth step on STEP. Holding OCT+ and turning knob 1 clockwise writes
ties into every step crossed after the selected step. OCT− writes rests instead.
The knob help stays visible for the entire OCT hold and clears on release or leaving STEP. Temporary warnings take priority, then the help returns if OCT is still held.
The starting step stays intact; movement stops at the pattern's end and backward
movement does not erase. Each hold starts an undo session. EDIT + OCT−/OCT+ restores
or reapplies its steps. Playing octave does not change while these modifiers are
pressed on STEP. Painting is disabled for recording, armed entry, free takes and
drums; other pages retain their normal OCT actions.

## Moving an event

Hold either OCT button and turn knob 3 left/right to move the selected note/chord
by whole steps. Selecting a tie finds its preceding onset and edits that event.
Movement clamps at pattern boundaries without wrapping.
Moving forward into the event's own tie run moves its onset later and keeps its original end; the vacated prefix becomes clean rests. Onset timing, conditions and locks move to the new onset, with onset locks taking precedence over matching destination locks. The remaining ties retain their metadata. Other moves shift the entire chain. Destination cells overlapping the moving chain are allowed; other cells must be
rests without notes, dynamics, flags, locks, fill conditions or timing data.
Otherwise STEP OCCUPIED leaves the pattern and cursor unchanged.

An existing note/chord onset is now a merge destination. Unique moved pitches are
added up to the four-note limit; duplicates do not use an extra slot. Destination
velocity, accents, ratchet, timing, fill, slide flags and locks remain authoritative.
A single-step source takes the destination chord's duration. A tied source can
merge when both chains have the same length; mismatched tied lengths or source
parameter locks reject before changing anything. Full chords reject a new fifth
pitch. The vacated source becomes clean rests and complete undo/redo includes both
ends. Selecting a source chord moves its whole onset, not an individual chord voice.

Notes, velocity, accents, slides, ratchets, microtiming, fill conditions and parameter
locks travel together. Vacated cells become clean rests. Direction-aware copying
handles overlapping moves without allocating a pattern buffer. Moving works
during playback; active recording, armed entry and free takes retain ownership
of pattern writes and show STOP RECORDING. Complete one-gesture
undo uses the existing pattern snapshot and shared starter metadata supplement.
Other tracks, sounds, tempo, length and division remain unchanged.

## Editing while the groove plays

Playback can continue while editing NOTES pitches/lengths, using OCT-held octave
and move controls, painting ties/rests, or applying NOTE/HIT ACTIONS (including
copy, move, delete, nudge, accent, fill and ratchet settings). No STOP FIRST is
required for these edits. Recording, armed entry and free takes keep their write
ownership; stop/disarm the take before these structural controls.

An already sounding attack keeps its existing gate and pitch until normal
sequencer reconciliation at the next step. Unchanged tied pitches continue
without reattack; removed or replaced voices release without silencing the rest
of the chord or other tracks. Future attacks read the edited pattern, including
on the next loop. There is no track panic or transport reset.

A successful edit cancels any remaining ratchet repeats of the current step on
that track. This prevents a changed four-note cell from adding an untracked fifth
pitch to the old sounding list. Ratchets resume normally on the next step; the
current attack and other tracks retain their normal gates. This also applies to
live step entry and EDIT deletion, which were already allowed during playback.

`tests/live_note_editing_test.c` runs actual UI operations with the audio engine
and MIDI queue active: four-voice pitch replacement under ratchets, independent
length shortening, move/copy/delete paths, next-loop playback, unchanged chord
voice and neighboring track continuity, and balanced note-on/note-off cleanup.

## Independent recording snap

SEQ navigation is STEP → PATTERN → RECORD → SEQUENCES → SONG. RECORD knob 1 is
SNAP: TRACK, 1/8, 1/4. Knobs 2–4 remain DIV, LEN and GATE. SNAP is a per-track,
process-only preference until reboot, retained across project selection. It does
not alter saved patterns, playback division or free-take conversion.

Live recording chooses the nearest coarse transport position with the existing
input-latency compensation and applicable swing, then converts it to a playback
step index. A requested grid must be at least as coarse as playback DIV and an
integer multiple of its duration. Incompatible/finer grids use normal track
quantization; the UI explicitly shows DIV LIMIT. TRACK is the default.

Future snapped notes keep the existing first-pass retrigger suppression. Held-note
recording writes no ties before that future onset. For an onset snapped behind the
current step, held duration is filled through the current step; subsequent chord
tones do not duplicate those ties. Normal held-note release and overdub protections
still apply. Synth and drum onsets share the same target calculation.

## Verification

`tests/seq_navigation_test.c` drives actual panel input with audio between frames:
painted runs across banks, source preservation, clamping, backward movement,
independent holds and undo/redo; overlapping chord/tie moves with timing/conditions/
locks, collision refusal, track isolation and complete metadata undo.
`tests/seq2_test.c` checks independent eighth/quarter targets, future/past held
onsets, chord grouping and loop-boundary recording, alongside the existing transport,
overdub, microtiming and arpeggiator regressions.

## Whole-octave pitch editing

For the entire synth pattern, hold EDIT and turn knob 4: each increment shifts all
stored notes one octave. Knob 3 still shifts semitones. The shift is uniform across
all notes and stops at MIDI bounds, preserving chord intervals, ties and metadata.
Drum tracks ignore these pitch controls.

Held EDIT shows SHIFT in steps, total TRANSPOSE in semitones and the octave-knob
amount for the current/last EDIT undo gesture. A new gesture starts fresh; undo
shows zero and redo restores the values. Another edit superseding that undo clears
the readout. The synth grid is a scale guide with physical-key hints, while actual
erase mapping stays unchanged. `tests/edit_tools_test.c` covers E-minor F# and
C-minor Eb labels, unchanged physical erase behavior, octave/step edits, gesture
totals, undo/redo, pitch bounds, metadata and neighboring-track isolation.

Hold either OCT button and turn knob 2 to shift the selected note/chord in whole octaves. A selected tie resolves to its preceding onset. All pitches move together; MIDI bounds clamp the number of octaves without collapsing intervals. Dynamics, ties, timing, conditions and locks remain intact. One hold shares one undo session with the other STEP gestures. Recording, armed entry, free takes and drums do not use this control; ordinary unmodified knob 2 still edits semitones. OCT taps keep their existing STEP behavior.

## Clear the selected synth step

On STEP, choose the step with knob 1 and tap EDIT. The selected step becomes a
rest and the cursor advances. This clears the entire step, including all its chord
notes. Hold EDIT and tap OCT− to undo; OCT+ reapplies. OCT− plus knob 1 is the rest
brush when clearing a longer range.

The STEP roll displays every sustained chord voice across bank boundaries. Its
knob feedback keeps the roll visible. During a tie gesture, NOTE and LEN stay on
the source (`SRC`, or `SEL` for a chosen pitch), even if shortening leaves the
cursor on an empty step. Normal STEP movement clears that source selection.
