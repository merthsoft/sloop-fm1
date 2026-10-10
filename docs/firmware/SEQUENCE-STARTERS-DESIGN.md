# Native musical sequence starters

The FM-1 can generate an editable four-bar pattern on the selected synth track without a phone. A ROM bank stores four scale degrees and a sixteen-step rhythm mask per starter, rather than sixty-four complete steps. It shares the drum groove rhythm mapper and supplemental undo snapshot.

## Bank and rendering

Browser choices are shared across synth tracks and retained until power-off: key,
scale, octave, starter, mode, voice leading, independent rhythm selection and rhythm shaping. The first native
visit seeds key/scale and VLEAD from the selected track, and octave from the physical
octave; later visits do not overwrite those choices. This lets chord, bass and arp
parts use the same key without changing any track's own SEL parameters. Preview
ownership still ends on exit or track changes; remembering settings does not keep
old audition notes sounding.

Twenty-four starters cover I–V–vi–IV, I–vi–IV–V, ii–V–I, a minor journey, soul sevenths, Dorian pocket, offbeat house, broken ii–V, funk side steps, a descending-fifths walk, a seventh-chord skip pattern, and floating Lydian movement. Merthsoft.6 adds GOSPEL TURN, MINOR DESCENT, SIX TWO FIVE, SOUL DETOUR, DEEP TWO CHORD, DISCO LIFT, GARAGE SKIPS, LATIN TURN, ODD POCKET, SUSPENSE, RISING STEPS and FALLING HOME. Names describe useful pairings; selecting a starter does not silently change scale. Dorian, minor and Lydian starters work particularly well with those scales.

Knobs on the main browser page choose starter, root, scale, and rendering mode. Root and scale begin with the selected track's settings, except a CHR track starts the browser in Major. This does not change track parameters; CHR remains explicitly selectable. CHR uses the actual twelve-note scale, not an implicit minor fallback. Stacking every other chromatic degree yields whole-tone intervals, so CHR is useful for unusual material rather than ordinary default progressions. Chord tones stack every other scale degree, including on pentatonic and other scales.

SELECT visits main → rhythm → pitch before SONG; tap SEQ skips browser subpages and opens SONG. Pitch-page knobs are OCTAVE, ROOT, SCALE and VLEAD. Browser octave starts at the physical octave, then changes independently; its displayed MIDI octave is 4 plus the offset, range 1–7. Preview and apply use this same explicit value. OCT−/OCT+ remain listen/apply. VLEAD starts from the selected track's preference without changing it.

VLEAD anchors the first bar at the requested register and derives each later bar's nearest inversion/octave from the preceding voicing. It shares the live chord helper's policy: minimum total voice motion, octave candidates −1/0/+1, MIDI 24–108 bounds, deterministic tie breaking. Every render derives from the first bar, so repeated preview loops do not drift or alter live chord history. CHORD uses the voiced chord; ARP NOTES walks its tones in sorted attack order; BASS remains on lower roots. The fixed first-bar anchor can produce a larger return at the loop boundary. It does not attempt contrapuntal rules or semantic harmony following.

CHORD renders triads or sevenths. With RHY = ORIGINAL, the sustained POP FOUR, CLASSIC TURN, and MINOR JOURNEY starters hold their chord through ties; rhythmic starters use separated stabs. BASS renders one root an octave lower. With RHY = ORIGINAL, ARP NOTES adds an eighth-note pulse while retaining the starter's syncopated attacks and cycles through each chord's three or four tones in attack order. It creates editable notes without changing the instrument's ARP settings. This keeps sparse starters from producing only roots in both BASS and ARP modes. Existing voice mode, engine voice limits, envelopes, sequencer gate, and sequence-fed arpeggiator behavior still apply. Choose a polyphonic voice mode to hear complete chords.

## Rhythm page

On any browser page, turn **PRESET** to select RHY independently of the progression.
ORIGINAL preserves starter behavior; explicit masks set attack spacing for all three modes.
See [independent rhythm selection](#independent-rhythm-selection) below.

SELECT cycles through the main browser, rhythm and pitch pages. Rhythm knobs choose rotation, offset, syncopation, and feel. Positive rotation and offset move events later, wrapping within the sixty-four-step pattern. The musical browser targets the whole track, so rotation and offset add together; the drum browser can offset a selected lane independently. Syncopation swaps eligible on-beat attacks into the preceding unoccupied step, preserving event count and metadata. Feel writes the existing per-step micro timing field, from half a step early to just under half a step late.

Sustained chord ties follow mapped steps. They carry no independent note pitches; shifting an attack can bring its harmonic change before the nominal bar boundary. Applied patterns use the real sequencer's micro timing. Native preview auditions the same mapped attacks, gate/tie behavior, and feel offset. Negative feel wraps the initial attack into a short lead-in, since an audition cannot sound before the listen button is pressed; subsequent events keep the exact interval and offset.

## Listen, apply, and safety

OCT− starts or stops a local preview. It uses the selected track's current sound and tempo without replacing any track data or starting transport. It releases only its own note list when stopped. Starting an audition is refused with RELEASE FIRST while the selected track already owns held ARP notes, a chord latch, or an ARP chord latch, even when the physical keys have been released. The existing latch and sounding voices remain untouched. A live piano key, transport start, recording, free take, panic, track switch, or browser exit ends preview. Preview runs before keyboard input so a new live press takes ownership afterward. The tiny preview state holds a clock, four sounding notes, and a frozen browser configuration; it allocates no full pattern buffer.

OCT+ applies immediately to an empty track. Existing notes, micro timing, fill conditions, or parameter locks require a second press or a continuous 700 ms hold. Changing browser settings cancels the pending confirmation. Apply is refused during playback, pending transport, recording, or free take.

Apply replaces only the selected synth track's sixty-four steps, length, division, swing, micro timing, fill, locks, and sequence-active flag. Division becomes sixteenth notes, swing becomes zero, locks and fill conditions clear, and feel becomes the chosen micro timing. Preset, engine, sound parameters, other tracks, global tempo, sections, and song chain stay unchanged. Existing track undo stores steps and length; the shared starter supplement stores other replaced metadata. EDIT+OCT−/OCT+ restores and reapplies the complete pattern, and later unrelated edits supersede that one-level snapshot.

## Integration and verification

Implementation lives in `src/sequence_starters.c` and `src/ui_sequence_starters.c`. The first is included after the shared starter undo definitions in `ui.c`; the browser is included after `ui_studio.c`. Main UI routing owns entry/exit. The native SEQ family exposes a SEQUENCES page before SONG for synth tracks; the browser registers preview tick/end callbacks when opened.

`tests/sequence_starters_test.c` checks all 24 generators and modes, exact major/minor transposition, note bounds, root and octave, sustained ties, rotation equivalence, syncopation, stopped guards, selected-track isolation, replaced metadata, and preview release on live-key takeover. It uses real core types and the real rhythm helper; only IRQ, note output, and undo capture are doubled. `tests/sequence_starters_ui_test.c` uses the real framebuffer UI, audio, sequencer, and shared undo implementation. It checks native entry, 700 ms apply thresholds, complete metadata undo/redo, untouched neighboring tracks, non-destructive preview, STOP and HOME release, and physical PLAY, SEQ, and SELECT routing. The broader UI and project suites cover track changes and project adoption. Hardware listening remains a user check.

Android Sequence now exposes the actual 24-entry firmware bank via additive command 76 (schema 1, INFO protocol 14). Discovery reads capabilities, starter names and scale names from the connected firmware. The phone contains no duplicated bank or generator. Its options cover key, scale, octave, the same three modes, VLEAD and rotation/offset/syncopation/feel. Audition runs on firmware with a 1500 ms renewable owner lease; apply is stopped-only, guards actual selected track and live ownership, asks explicit replacement confirmation, and uses complete native hardware undo. See [wire contract](MUSICAL-STARTER-PROTOCOL.md).

Turn the SELECT knob/encoder to navigate main → rhythm → pitch; the physical SEL button is a separate control. Internal SELECT page navigation preserves the running audition and its phase; configuration knob changes cancel preview, and leaving the browser stops it.

## Independent rhythm selection

Turn the physical **PRESET encoder** on any SEQUENCES browser page to choose
**RHY**, independently of the progression. ORIGINAL preserves each starter's
built-in rhythm, including ARP NOTES' additional eighth-note pulse. The 17 explicit
choices are BAR, HALVES, QUARTERS, EIGHTHS, SIXTEENTHS, OFFBEATS, SOUL, DORIAN,
BROKEN, FUNK, SKIPS, FLOAT, GARAGE, LATIN, PUSHES, FALLING and SOUL DETOUR.
These masks repeat over the four harmony bars. Explicit choices use exactly their
attacks in all three modes; ARP NOTES cycles chord tones at those attacks instead
of adding extra pulses. BAR sustains chord mode with ties; bass/arp use the normal
note gate. VLEAD still controls inversions independently.

Rotate, offset, syncopate and feel modify the chosen rhythm. FEEL is a uniform
micro-timing nudge, in 1/64-step units: -32 is half a step early, +31 just under half
a step late. It is neither swing nor a random/humanize control. Preview and Apply
share the selection. Changing rhythm cancels audition; changing SELECT pages does
not. The choice persists across track/browser changes until reboot.

The existing phone starter protocol continues to request ORIGINAL. A phone request
neither inherits nor overwrites the native RHY preference; exposing the rhythm bank
in Android is future work.
