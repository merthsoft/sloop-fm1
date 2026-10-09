# Native musical sequence starters

The FM-1 can generate an editable four-bar pattern on the selected synth track without a phone. A ROM bank stores four scale degrees and a sixteen-step rhythm mask per starter, rather than sixty-four complete steps. It shares the drum groove rhythm mapper and supplemental undo snapshot.

## Bank and rendering

Twelve starters cover I–V–vi–IV, I–vi–IV–V, ii–V–I, a minor journey, soul sevenths, Dorian pocket, offbeat house, broken ii–V, funk side steps, a descending-fifths walk, a seventh-chord skip pattern, and floating Lydian movement. Names describe useful pairings; selecting a starter does not silently change scale. Dorian and Lydian starters work particularly well with those scales.

Knobs on the main browser page choose starter, root, scale, and rendering mode. Root and scale begin with the selected track's settings; changing them in the browser affects the generated notes, leaving the sound's parameters untouched. The current physical octave selects register. CHR uses the actual twelve-note scale, not an implicit minor fallback. Chord tones stack every other scale degree, including on pentatonic and other scales.

CHORD renders triads or sevenths. The sustained POP FOUR, CLASSIC TURN, and MINOR JOURNEY starters hold their chord through ties; rhythmic starters use separated stabs. BASS renders one root an octave lower on the original rhythm. ARP NOTES adds an eighth-note pulse while retaining the starter's syncopated attacks and cycles through each chord's three or four tones in attack order. It creates editable notes without changing the instrument's ARP settings. This keeps sparse starters from producing only roots in both BASS and ARP modes. Existing voice mode, engine voice limits, envelopes, sequencer gate, and sequence-fed arpeggiator behavior still apply. Choose a polyphonic voice mode to hear complete chords.

## Rhythm page

SELECT changes between the main browser and rhythm pages. Rhythm knobs choose rotation, offset, syncopation, and feel. Positive rotation and offset move events later, wrapping within the sixty-four-step pattern. The musical browser targets the whole track, so rotation and offset add together; the drum browser can offset a selected lane independently. Syncopation swaps eligible on-beat attacks into the preceding unoccupied step, preserving event count and metadata. Feel writes the existing per-step micro timing field, from half a step early to just under half a step late.

Sustained chord ties follow mapped steps. They carry no independent note pitches; shifting an attack can bring its harmonic change before the nominal bar boundary. Applied patterns use the real sequencer's micro timing. Native preview auditions the same mapped attacks, gate/tie behavior, and feel offset. Negative feel wraps the initial attack into a short lead-in, since an audition cannot sound before the listen button is pressed; subsequent events keep the exact interval and offset.

## Listen, apply, and safety

OCT− starts or stops a local preview. It uses the selected track's current sound and tempo without replacing any track data or starting transport. It releases only its own note list when stopped. Starting an audition is refused with RELEASE FIRST while the selected track already owns held ARP notes, a chord latch, or an ARP chord latch, even when the physical keys have been released. The existing latch and sounding voices remain untouched. A live piano key, transport start, recording, free take, panic, track switch, or browser exit ends preview. Preview runs before keyboard input so a new live press takes ownership afterward. The tiny preview state holds a clock, four sounding notes, and a frozen browser configuration; it allocates no full pattern buffer.

OCT+ applies immediately to an empty track. Existing notes, micro timing, fill conditions, or parameter locks require a second press or a continuous 700 ms hold. Changing browser settings cancels the pending confirmation. Apply is refused during playback, pending transport, recording, or free take.

Apply replaces only the selected synth track's sixty-four steps, length, division, swing, micro timing, fill, locks, and sequence-active flag. Division becomes sixteenth notes, swing becomes zero, locks and fill conditions clear, and feel becomes the chosen micro timing. Preset, engine, sound parameters, other tracks, global tempo, sections, and song chain stay unchanged. Existing track undo stores steps and length; the shared starter supplement stores other replaced metadata. EDIT+OCT−/OCT+ restores and reapplies the complete pattern, and later unrelated edits supersede that one-level snapshot.

## Integration and verification

Implementation lives in `src/sequence_starters.c` and `src/ui_sequence_starters.c`. The first is included after the shared starter undo definitions in `ui.c`; the browser is included after `ui_studio.c`. Main UI routing owns entry/exit. The native SEQ family exposes a SEQUENCES page before SONG for synth tracks; the browser registers preview tick/end callbacks when opened.

`tests/sequence_starters_test.c` checks all twelve generators and modes, exact major/minor transposition, note bounds, root and octave, sustained ties, rotation equivalence, syncopation, stopped guards, selected-track isolation, replaced metadata, and preview release on live-key takeover. It uses real core types and the real rhythm helper; only IRQ, note output, and undo capture are doubled. `tests/sequence_starters_ui_test.c` uses the real framebuffer UI, audio, sequencer, and shared undo implementation. It checks native entry, 700 ms apply thresholds, complete metadata undo/redo, untouched neighboring tracks, non-destructive preview, STOP and HOME release, and physical PLAY, SEQ, and SELECT routing. The broader UI and project suites cover track changes and project adoption. Hardware listening remains a user check.

No Android UI or new wire command is included. These controls can later be surfaced in the app using a dedicated versioned command, with the same firmware renderer as the source of truth.



