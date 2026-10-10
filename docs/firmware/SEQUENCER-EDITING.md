# Native recording snap and STEP editing

Recording snap and tie/rest painting shipped in 2.5 Merthsoft.6. The controls below
reflect Merthsoft.7, which adds octave edits, moves event editing to knob 3 and allows
forward moves into an event's own ties. No wire parameter IDs or project formats change.

## Painting lengths and silence

Select a synth step on STEP. Holding OCT+ and turning knob 1 clockwise writes
ties into every step crossed after the selected step. OCT− writes rests instead.
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
