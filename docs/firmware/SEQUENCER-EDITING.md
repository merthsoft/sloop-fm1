# Native recording snap and STEP editing

Implemented in 2.5 Merthsoft.6. No wire parameter IDs or project formats change.

## Painting lengths and silence

Select a synth step on STEP. Holding OCT+ and turning knob 1 clockwise writes
ties into every step crossed after the selected step. OCT− writes rests instead.
The starting step stays intact; movement stops at the pattern's end and backward
movement does not erase. Each hold starts an undo session. EDIT + OCT−/OCT+ restores
or reapplies its steps. Playing octave does not change while these modifiers are
pressed on STEP. Painting is disabled for recording, armed entry, free takes and
drums; other pages retain their normal OCT actions.

## Moving an event

Hold either OCT button and turn knob 2 left/right to move the selected note/chord
by whole steps. Selecting a tie finds its preceding onset and moves that entire
onset/tie chain. Movement clamps at pattern boundaries without wrapping.
Destination cells overlapping the moving chain are allowed; other cells must be
rests without notes, dynamics, flags, locks, fill conditions or timing data.
Otherwise STEP OCCUPIED leaves the pattern and cursor unchanged.

Notes, velocity, accents, slides, ratchets, microtiming, fill conditions and parameter
locks travel together. Vacated cells become clean rests. Direction-aware copying
handles overlapping moves without allocating a pattern buffer. Moving requires
stopped transport and no recording/armed/free-take operation. Complete one-gesture
undo uses the existing pattern snapshot and shared starter metadata supplement.
Other tracks, sounds, tempo, length and division remain unchanged.

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
