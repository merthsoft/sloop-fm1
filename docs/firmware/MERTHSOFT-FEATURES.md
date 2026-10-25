# Playing the MerthSloop firmware

This guide covers the additions to upstream SLOOP in the `merthsloop` branch,
including the current scale guides and EDIT pitch tools. Everything below works without
a phone. The [complete player guide](../../GUIDE.md) also covers upstream features;
the [Android overview](../../README.md#merthsloop-mobile-for-android) covers the companion.

**SEL** is the physical button between FX and ENV, historically labelled SCL in
firmware. **SELECT** is the separate rotary encoder. They do different things.

## Filter, punch and step-entry preferences

Hold **HOME**, then use **SELECT** to find **FX** or **SEQ** in the menu.

| Menu | Knob 1 | Knob 2 |
| --- | --- | --- |
| FX | FILTER RESO, 0–10 | PUNCH FADE, 0–15 |
| SEQ | STEP ADVANCE, ON/OFF | DRUM PREVIEW, ON/OFF |

| FX preferences | SEQ preferences |
| --- | --- |
| <img src="../../assets/screens/merthsoft/neon/menu-fx-controls.png" width="240" alt="NEON menu with filter resonance and punch fade"> | <img src="../../assets/screens/merthsoft/neon/menu-seq-controls.png" width="240" alt="NEON menu with step advance and drum preview"> |

FILTER RESO adds resonance to synth/drum track filters, independently of wah and
the master DJ filter. PUNCH FADE controls effect engagement/release from the
original fast anti-click ramp to about 0.37 seconds. Holding **FX** and turning
**SELECT** also adjusts it, with the value shown below the effect keys. It covers
primary effects, punch filters, gate engagement and the temporary LOFI boost.
The gate's rhythm and HALF/DOUBLE transport gestures stay immediate.

STEP ADVANCE defaults ON. Turn it OFF to keep the cursor in place after releasing
a played chord; the next chord starts a fresh replacement with its own undo.
DRUM PREVIEW defaults OFF and controls audition when browsing lanes/steps on GRID
and SOUND. Explicit grid entry, groove HEAR/preview and stopped TUNE/DECAY audition
remain available. These preferences save on the device rather than in a song.

For noise-based drum sounds, SOUND → TUNE now shifts filter brightness as well as
the existing pitch controls for tonal sounds. Factory and user synth kits use the
same behavior; tuning at zero retains the original sound.

## NEON display

Hold **HOME → SCREEN → COLOR** and choose **NEON**. New settings default to it;
existing saved colors and editor-created themes remain available. Four knob cards
follow the physical controls across ordinary editing, held-button maps, live
modulation and native library screens. The overview shows four track lanes, with
selected/muted tracks and sequencer position; the striped sun pulses gently
with the transport beat while playing. Sparse stars, a pulsing horizon glow and a
beat-driven wireframe highlight add movement without a new animation buffer. Page footers identify track/global
scope and the action available now. Existing shortcuts, scale-key LEDs, independent
ties, preview, merge and replacement behavior are retained.

**PAGE HELP**, the fourth SCREEN item, opens a reference for the underlying page.
Use PRESETS to select it and OCT+ to open; OCT− returns to the menu.
SONG and record-ready/count-in use the same knob cards, palette, transport header
and footer spacing. HOME identifies the selected drum kit by name alongside
the colored DRUMS label, just as synth tracks show their preset and engine.
Disabled controls show muted dashes. Running free takes keep
a large timer and the four-track overview. Existing recording/saving gestures
are unchanged.
PRESETS is the left control, ALGORITHM the right. HOME shows the engine name in
the selected track color beside the preset. LFO/ENV hold hints identify vibrato
and tremolo instead of a generic actions label.

Envelope and LFO diagrams, signed destination amounts, filter/delay guides, live
vibrato/tremolo traces, stereo pan and the wah sweep guide replace ambiguous empty
areas. The diagrams are setting/activity guides, not measured frequency responses.

| Overview | Modulation | Native libraries |
| --- | --- | --- |
| ![NEON HOME](../assets/screens/merthsoft/neon/home.png) | ![NEON pan](../assets/screens/merthsoft/neon/pan.png) | ![NEON groove browser](../assets/screens/merthsoft/neon/groove.png) |

The [design and implementation record](UI-REFRESH-DESIGN.md) contains more actual
screens. Older detailed screenshots below may show the previous layout; controls
are unchanged.

### Eleven black-key chord modifiers

SEL's chord guide now names all eleven physical black keys. The lower five keep
third flip, seventh, sus4, ninth and upward inversion. The upper six add **sus2,
thirteenth, explicit major/minor sevenths, downward inversion and open voicing**.
Hold them momentarily or tap to toggle with chord LATCH; retained arps revoice
without replaying the root. Automatic voice leading does not cancel register
choices. Four voices remain the limit, with extensions taking the fifth's place.

CHORD OFF and CHROM show the normal scale guide. These physical mappings stay
fixed when the playing octave changes. See the [key table and combination rules](CHORD-MODIFIER-EXPANSION.md).

## Perform a progression

Choose a stock or CUSTOM progression in SEQ's SEQUENCES browser, then turn SELECT
to its fifth page, PERFORM. Turn LIVE on while stopped or during loop playback. The leftmost white keys select the progression's chords; the next group repeats
them another octave higher for each group, across all sixteen white keys. HOLD sustains the last selection. OCTAVE changes the next
chord without retuning the one currently playing. ARP OFF plays sustained chords; with ARP ON the existing arp supplies the
rhythm; PLAY runs accompanying tracks and REC captures its generated notes.
Stored notes on the live track stay intact and silent while you perform.

![Native progression performance](../assets/screens/merthsoft/neon/progression-perform.png)

Black keys use the same eleven modifiers as ordinary chord mode, momentarily or
as toggles with HOLD on. Both sustained chords and arps respond. This mode uses
immediate changes and diatonic triads/sevenths; queued beat/bar changes and
starter-rhythm performance remain future work.
[Controls, recording and lifecycle](PROGRESSION-PERFORM-DESIGN.md).

## Saving sections safely

Hold SAVE and use SAVE A–D (keys 5–8); confirm a used section with a second tap.
When stopped, this now writes the section to flash before showing SAVED SECTION.
It also refreshes the startup working-loop snapshot and commits device settings,
including the 1/8 swing-grid preference. Track root/scale, levels, drum level and
global swing belong to the saved section. Explicit SAVE does not wait for the
background autosave interval before protecting the power-on state.

While playing, it captures a snapshot without interrupting audio and says
**QUEUED: STOP TO SAVE**. Stop playback, release sounding notes, and wait for
**SECTIONS SAVED TO FLASH** before powering off. The queued section and startup
snapshot flush as soon as playback stops and voices are quiet; routine settings
writes retain their idle delay. Failed writes remain pending
and retry; RAM-only storage is explicitly labeled. SONG-screen SAVE still saves
the arrangement order; REC stores its selected section.

## Build a loop in a minute

1. Select drums. Tap EDIT or SEQ to reach **GROOVE**, choose a beat, and press
   **OCT−** to listen through the current kit. Press OCT− again to stop preview.
2. Press **OCT+** to apply. Replacing existing material requires another press or
   a continuous **700 ms hold**. Playback and recording must be stopped.
3. Select a synth. Tap SEQ through **STEP → NOTES → NOTE ACTIONS → PATTERN → RECORD → SEQUENCES**.
   Select POP FOUR, your key and scale, and **ARP NOTES**. Listen, then apply.
4. On another synth, apply the same progression in **BASS** mode. Add a melody,
   choose sounds and develop the ordinary editable steps you have just created.

| --- | --- |
| <img src="../../assets/screens/merthsoft/guide/feature-groove-main.png" width="240" alt="FOUR FLOOR drum groove browser"> | <img src="../../assets/screens/merthsoft/guide/feature-starter-main.png" width="240" alt="POP FOUR progression in the native sequence browser"> |

There are **24 grooves**, including funk, broken house, electro, Afro clave,
boom bap and a four-bar Amen break, and **27 musical starters**, including standard
progressions, soul sevenths, gospel turns, disco, garage, Latin and unusual pockets.
The libraries preserve your kit/preset, tempo and other tracks. Applied material
replaces the selected track's pattern and timing metadata; **EDIT + OCT−** undoes
the complete replacement and **EDIT + OCT+** redoes it while stopped.



## Swing eighth-note grooves

Hold **HOME → TIMING → SWING GRID**, then choose **1/8** with knob 1. Return to
TRACKS and turn **SWING8** to set the amount. This shuffles eighth-note hats such
as HALF TIME even when the pattern uses a 1/16 grid. **STEP** preserves the old
swing behavior, following each track's DIV. The choice survives power-off; both
global and local swing amounts follow it. Native previews and the GROOVE
visualizer follow the timing too. Live ARP retains its separate ARP 2 SWG control.

![Eighth swing setting](../../assets/screens/merthsoft/guide/swing-grid-eighth.png)

## Write a custom progression

In SEQUENCES, choose **CUSTOM** after the built-in starters with knob 1, then turn
SELECT to the fourth browser page, **CUSTOM**. Its controls are **1 COUNT** (1–8),
**2 SLOT**, **3 DEGREE** (1–7), and **4 SPAN** (4/8/16 sixteenth-note steps per
chord). The underline marks the edited position. Dial `5–1–2–4`, or choose three,
six or eight positions; changing count retains hidden slots for later reuse.
Editing count, degree or span selects CUSTOM automatically.

The 64-step limit applies to the whole phrase. More than four chords limits SPAN
to eight steps. Three chords at span 16 make 48 steps; six at span 8 also make 48.
The footer shows steps per chord. Root, scale, octave, rhythm, shaping, VLEAD,
CHORD/BASS/ARP NOTES, listen, ADD/REPLACE and undo work as for the factory starters.
Preview and REPLACE use the actual phrase length. ADD keeps the destination length.
The shared custom draft lasts until power-off; applied notes save with the song.

![Native custom 5–1–2–4 progression](../../assets/screens/merthsoft/guide/custom-progression.png)
## Shape the rhythm and register

Turn **SELECT** inside SEQUENCES to visit its main, rhythm and pitch pages.
Key, scale, octave, starter, mode, VLEAD and rhythm choices stay shared across synth
tracks until power-off, so matching chord, bass and arp parts keep the same key.
Page changes preserve the audition; changing a configuration knob stops it.
The physical SEL button opens chord settings instead. Tapping SEQ leaves the
browser for SONG; that final screen belongs to the sequence family.

| Sequence rhythm | Sequence pitch | Drum RHYTHM |
| --- | --- | --- |
| <img src="../../assets/screens/merthsoft/guide/feature-starter-rhythm.png" width="240" alt="Rotation offset syncopation and feel knobs"> | <img src="../../assets/screens/merthsoft/guide/feature-starter-pitch.png" width="240" alt="Octave root scale and VLEAD controls"> | <img src="../../assets/screens/merthsoft/guide/feature-groove-shape.png" width="240" alt="Drum lane shift syncopation and feel"> |

| Page | Knob 1 | Knob 2 | Knob 3 | Knob 4 |
| --- | --- | --- | --- | --- |
| SEQUENCES main | Starter | Root | Scale | CHORD / BASS / ARP NOTES |
| Sequence rhythm | Rotate | Offset | Syncopation | Feel |
| Sequence pitch | Octave | Root | Scale | VLEAD |
| Drum RHYTHM | Target lane/all | Shift | Syncopation | Feel |

Positive shifts move later, wrapping within the pattern. Syncopation moves eligible
on-beat attacks into the preceding empty step; occupied destinations stay intact.
Feel nudges timing early or late independently of swing. Drum lane shifts can
put hats behind the kick; drum feel uses the existing shared per-step timing.
Preview and Apply use the same configuration, rebuilt from the original starter.

The pitch page's **OCTAVE** is independent of OCT−/OCT+, which remain listen/apply.
**VLEAD** keeps later chords near the preceding voicing, anchoring the first bar at
your chosen register. It applies to CHORD and ARP NOTES; BASS stays on lower roots.
Preview loops do not drift in octave. Choose a polyphonic VOICE mode for full chords.

**CHORD** writes triads/sevenths with the selected rhythm; **BASS** writes lower
roots. **ARP NOTES** writes editable chord-tone pulses. ORIGINAL adds eighth-note
pulses and retains starter syncopations; an explicit RHY uses only its own attacks. It does not enable the live arpeggiator. A track set to CHR
opens this browser in Major without changing the track's scale settings.
Explicit CHR here means twelve chromatic degrees: stacked chord tones sound
unusual. Use MAJ/MIN or another tonal scale for conventional progressions.

See [musical starters](SEQUENCE-STARTERS-DESIGN.md), [drum grooves](DRUM-GROOVES-DESIGN.md)
and [rhythm shaping](RHYTHM-SHAPING-DESIGN.md) for bank and replacement details.

## Sustain individual notes through later attacks

Select a source note/chord on STEP. Hold **OCT+** and turn **knob 4** to choose
ALL or one source pitch; scrub **knob 1** forward to extend it. Other pitches can
attack underneath the sustained voice. Set **EDIT → VOICE → knob 1 to POLY**.
After releasing OCT+, **LEN** shortens the selected voice and **EDIT** deletes it
and its continuations without removing the other voices. Undo restores the edit.

<img src="../../assets/screens/merthsoft/guide/per-note-ties-voice.png" width="320" alt="Native STEP editor with an independently selected sustained voice">

A full destination or a deliberate same-pitch attack stops extension safely.
Single-event move/transpose is guarded for independently tied patterns; whole-pattern
EDIT transpose remains available. Recording and phone/web duration authoring remain
follow-on work. See [per-note ties](PER-NOTE-TIES-DESIGN.md) for details.

## Record and correct notes quickly

For the whole synth pattern, hold **EDIT** and turn **knob 4** for octaves;
knob 3 remains semitones. SHIFT/TRANSPOSE/OCTAVE readouts follow the last gesture
and its undo/redo. The scale guide shows key-signature accidentals while small
physical-key hints preserve the existing erase controls.

| EDIT shift and pitch totals | E-minor vibrato guide |
| --- | --- |
| <img src="../../assets/screens/merthsoft/guide/feature-edit-octave.png" width="240" alt="EDIT panel showing one-step shift, minus eleven semitones and minus one octave-knob increment"> | <img src="../../assets/screens/merthsoft/guide/feature-vibrato-scale-guide.png" width="240" alt="Vibrato E-minor guide with F-sharp and physical-key hints"> |

| RECORD snap | Sequencer-fed arp |
| --- | --- |
| <img src="../../assets/screens/merthsoft/guide/feature-record-snap.png" width="240" alt="Eighth-note record snap with sixteenth-note playback division"> | <img src="../../assets/screens/merthsoft/guide/feature-sequence-arp.png" width="240" alt="ARP 2 ORD set to SNOTE"> |

On **RECORD**, knob 1 **SNAP** can round recording to eighths or quarters while
the pattern keeps its own playback DIV. It is separate from changing track speed.

On the **STEP page**, select the starting note/chord, then:

| Gesture | Result |
| --- | --- |
| Hold OCT+ and scrub knob 1 clockwise | Paint following ties for as far as you advance |
| Hold OCT− and scrub knob 1 clockwise | Paint following rests |
| Hold either OCT button and turn knob 2 | Shift the selected note/chord by whole octaves |
| Hold either OCT button and turn knob 3 | Move its onset left/right by whole steps |

Tie/rest painting leaves the starting step intact, stops at the pattern end and
does not erase when backtracking. Moving into a note's own ties is allowed and
retains its original end; other moves shift the complete tie chain. Occupied
destinations are protected. Moves carry expression, timing, fill conditions and
parameter locks; gestures support undo/redo. These are STEP-page controls,
distinct from the held-SEQ layer. See [editing details](SEQUENCER-EDITING.md).

| Start with a chord on step 1 | Sweep ties through step 8 | Paint rests on steps 5–7 |
| --- | --- | --- |
| <img src="../../assets/screens/merthsoft/guide/tie-before.png" width="240" alt="C major chord at step 1 with following rests"> | <img src="../../assets/screens/merthsoft/guide/tie-after.png" width="240" alt="Following seven steps tied by OCT+ and knob 1"> | <img src="../../assets/screens/merthsoft/guide/rest-after.png" width="240" alt="Steps 5 through 7 replaced with rests using OCT- and knob 1"> |

The thin horizontal segments represent ties. For the rest example, select step 4,
hold OCT− and advance knob 1 to step 7; step 4 stays intact.

| Original onset at step 1 | Onset moved to step 3 inside its own ties |
| --- | --- |
| <img src="../../assets/screens/merthsoft/guide/move-before.png" width="240" alt="Chord attack followed by ties through step 8"> | <img src="../../assets/screens/merthsoft/guide/move-after.png" width="240" alt="Chord moved two steps later with its original end retained"> |

Hold either OCT button and turn knob 3 two steps right. Steps 1–2 become rests;
the chord begins at step 3 and still ends after step 8.

## Play chords and arpeggios

Added chord shapes are **SUS2, ADD9, 6TH, SHELL, OCTAVE, MAJOR, MINOR, DOM7,
MAJ7, MIN7, DIM, AUG, HALFDIM and DIM7**. They extend upstream's TRIAD, 7TH,
9TH, SUS4 and POWER. Fixed qualities are useful when every root should retain
the same chord type; scale-derived qualities follow the selected harmony.

Set **KEYS/QNT = CHROM** to use every physical key as a literal chord root,
including black keys. Other chord keyboard modes use white keys for degrees
and black keys for chord modifiers. CHR is a scale, while CHROM is a keyboard
mapping; legacy scale-derived live chords use minor harmony when SCALE is CHR.

While holding sounding notes, hold **ARP or SEL for 700 ms** to toggle latch
without interrupting your playing. This works for chord mode and manually held
arpeggio notes. **SEL page 2, knob 4** also controls chord LATCH. In non-CHROM
latched chord mode, black modifiers immediately reshape the sounding chord and
toggle until pressed again. Their state survives choosing another root and clears
when latch is disabled, CHROM is selected, STOP or panic is used.

<img src="../../assets/screens/merthsoft/guide/chord-latch-controls.png" width="240" alt="SEL page 2 with knob 4 LATCH ON">

With C major selected, latch the C chord, release its root, then tap **F#**:
**C–E–G → C–E♭–G**. Releasing F# keeps the minor chord; tapping F# again returns
to major. You do not need to reapply the root. The image shows the LATCH control;
the modifier changes the sounding pitches, not the saved CHORD setting.

Additional arp modes are **CONV, SHUF, ROOTALT, DNUP, UPDNREP, DIVG, WALK
and PULSE**. NOTE/PLAY ordering controls sorted pitch versus playing order;
ORD keeps press order. ROOTALT alternates the lowest voiced pitch. PULSE plays
the expanded distinct tones together, merging duplicate octave pitches.

To arpeggiate a sequenced chord, enable an arp MODE and set **ARP page 2,
knob 4 ORD to SNOTE or SPLAY**. Store the chord on one step with ties after it
to sustain the arp pool. Live and sequence input remain independent. Source
velocity/accent is retained through arp modes, octave expansion, latch, MIDI
output and recording; shared pitches use the stronger current source velocity.
Physical keys still have fixed attack velocity. The FM-1's shared eight-voice
budget and four-note-per-step recording limit continue to apply.

See [arp expression](ARP-EXPRESSION.md) and the [chord/arp architecture](CHORD-ARPEGGIO-DESIGN.md),
which distinguishes the implemented features from future semantic harmony work.

## Keep the key visible

Hold **SEL + press OCT+** to toggle persistent keyboard scale lights. They follow
the selected synth's SEL-page ROOT/SCALE, with dim scale notes and bright played
notes. Off-scale keys stay dark until played. CHR lights all twelve pitch classes.
The guide is visual: it does not itself quantize or transpose your playing.

<img src="../../assets/screens/merthsoft/guide/feature-scale-grid.png" width="240" alt="C minor grid with Eb Ab Bb labels">

Held SEL shows mapped pitches and appropriate accidentals; LFO/ENV show modulation waveforms with a compact key guide.
**KEYS = WHITE** walks scale degrees. Optional **HOME menu → MIDI SCALE → KEYBOARD** (knob 1) maps incoming USB/TRS synth pitches by that receiving track's
keyboard settings. It defaults OFF, useful for phones/DAWs already sending the
desired pitches. Note-offs retain their original mapping across setting changes.
This option maps individual pitches; it does not generate chord voicings or turn
incoming black keys into modifier commands. See [scale/MIDI grids](SCALE-MIDI-GRIDS.md).

## Vibrato and tremolo while playing

Hold **LFO** for vibrato or **ENV** for tremolo. After 140 ms a temporary panel
appears for the selected synth only; release returns to your previous screen.
Quick taps still open the normal saved patch pages. The keyboard remains playable
and retains its scale lights.

| Locked vibrato | Locked tremolo |
| --- | --- |
| <img src="../../assets/screens/merthsoft/guide/vibrato-locked.png" width="240" alt="Vibrato panel showing LOCK"> | <img src="../../assets/screens/merthsoft/guide/tremolo-locked.png" width="240" alt="Tremolo panel showing LOCK"> |

**To keep it on: hold LFO/ENV, tap HOME, then release LFO/ENV.** LOCK appears and
the effect and knobs stay active. Tap HOME to unlock. Another function button
also unlocks; press it again to open its normal page. STOP, track panic, menus
and selected-track changes clear the effect and dismiss the locked panel.
Unlock before selecting another track through the panel buttons. One panel locks
at a time; a lock lasts only for this performance, not across a project save/reboot.

The waveform shows one cycle and a live phase marker, with pitch cents for vibrato
and volume percentage for tremolo. Track/key context and a compact scale guide
remain visible; the keyboard keeps its existing mapping.

Knobs 1–4 are **free Hz, depth, waveform, beat sync**. SYNC offers OFF, 1/4, 1/8,
1/16, 1/32, 8T, 16T, 1/2, 1BAR and 2BAR. Turning knob 1 returns to free Hz.
Synced modulation follows transport phase when playing and current BPM when stopped.
The settings are independent of saved LFO/envelope parameters and remembered across
holds until reboot. Both buttons can be held together; LFO takes visible knob priority.

**MIDI CC1** also supplies vibrato through the saved track LFO, with source-aware
USB/TRS cleanup. Android Perform's momentary wheel strip sends it too. The hardware
panel overrides the wheel on its captured synth, then restores the current wheel
on ordinary release. Details: [performance modulation](MODULATION-WHEEL.md).

## Punch effects, fills and project safety

Hold **FX** and a keyboard key for one of **27 mix-wide punch effects**. The
sixteen white keys retain their loops, stutter, reverse, tape stop, half speed,
filters, phone, crush, alias, gate, echo and wobble. Eleven black keys add
**KILL LOW/MID/HIGH, REWIND, DOUBLE TIME, FLANGER, AUTO-PAN, FREEZE, LOOP 8T, VINYL
and FADE OUT**. FX + held **OCT− / OCT+** adds reverb / echo throws. Release returns
the normal mix; HOME can lock the FX control panel open, freeing the FX-button hand.
These act on the mix, whereas LFO/ENV modulation acts on one synth.
See the [physical effect mapping](../../GUIDE.md#punch-in-effects).

### Six reverb characters

**FX → REVERB → TYPE** offers the original **STEREO**, a dual-path **SPRING**,
short **ROOM**, bright dense **PLATE**, spacious **HALL**, and a dense muted **DARK** tail with compensated return level
for more presence at moderate sends. The new spring
uses unequal lengths, different dispersion and offset pickup modulation for
stereo width. The three spaces retune the existing feedback delay network;
they reuse its buffers and leave the original STEREO sound unchanged.

Knob 2 **SIZE** adjusts decay and knob 3 **DAMP** darkens the tail. SPRING alone
uses knob 4 **TIME** (20–80 ms) to change reflection spacing independently of
its nominal decay. Track REV sends remain independent. Type changes fade and
clear the old tail; FREEZE holds the current type until released. Types and
spring timing save with projects/sections. See [design and measured checks](REVERB-EXPANSION.md).

The old black-key modifier controls and their screenshot are historical. Current
black keys select effects directly, as in upstream; they do not toggle effect latch.
The phone's established performance protocol continues to address the sixteen
original effects. See [punch design and historical context](PUNCH-FX-DESIGN.md).

Held GLO's **FILL** key enables existing fill-conditioned steps; **FILL BAR** queues
the next bar. They do not invent a new drum beat. The phone can control these and
punch effects through renewable leases; physical controls take priority and expired
remote owners release cleanly.

After an accidental saved-song load, section selection or working-project backup
restore, stop playback and press **EDIT + OCT−** to restore the preceding project.
OCT+ redoes it. The one-level RAM snapshot is superseded by subsequent edits or
recording. The held SAVE menu also offers **NEW**, with stopped-only confirmation.

| Undo a load | SAVE menu's NEW | Confirm NEW |
| --- | --- | --- |
| <img src="../../assets/screens/merthsoft/guide/undo-load-controls.png" width="240" alt="Held EDIT panel showing oct- undo load and oct+ redo"> | <img src="../../assets/screens/merthsoft/guide/save-new.png" width="240" alt="Held SAVE menu with NEW in the third row"> | <img src="../../assets/screens/merthsoft/guide/save-new-confirm.png" width="240" alt="NEW highlighted red with AGAIN NEW confirmation"> |

For NEW, keep SAVE held and press the **ninth white key** twice within the
confirmation window. Releasing SAVE cancels confirmation. Creating a new working
project leaves saved flash slots intact.

## Visualizers, USB and the companion

On TRACKS, tap HOME to open the visualizer and turn SELECT to choose among **seven
styles**: Oscilloscope, Spectrum, Spectrogram, VU Meters, Note Trails, Groove
and Song Journey. The last three are fork additions. The [complete illustrated gallery](VISUALIZERS.md)
describes all available styles. Performance layers and transport remain usable.

Note Trails captures played/held notes rather than continuing released voices or
delay tails; the fully wet master punch GATE cuts its current synth/drum column.

**Apply modulation:** while holding LFO/ENV, tap OCT+ to copy rate, waveform, beat
division and PIT/AMP depth into the track's saved shared LFO. The temporary effect
stops. Other destination depths stay, sharing the new timing; vibrato depth rounds
to existing PIT steps. Save the project normally. See [modulation](MODULATION-WHEEL.md).

**LOFI chip arp:** MOTN → ARP retains its original four choices and adds every SEL
chord shape, using the track's root/scale and the existing chip cadence. See
[LOFI chord arpeggios](LOFI-CHORD-ARP.md).

**Phone song library:** Library → FM1 songs saves named current-project/A–D snapshots
with arrangement, samples, sound banks and edited FM6 voices. Load retains a phone
recovery song before replacement; device preferences and MIDI mappings stay.
See [song library](<../../android/design docs/FM1-SONG-LIBRARY.md>).

USB audio records the instrument into the phone/computer. App sample previews use
phone audio; playback into the FM-1 speaker/headphones is unavailable.

Companion protocol 15 provides hardware octave reporting, native pattern and FM6
editing, sample backup/upload/readback, persistent FM6 base-voice bank saving,
and canonical drum-library discovery. Upstream SYN-kit commands retain IDs 72–76;
remote performance uses 86 and the drum bank uses 87. Atomic live hardware scene switching remains future work.
Original Android code is Unlicensed with retained dependency obligations; firmware
remains GPL. See [current delivery status](<../../android/design docs/STATUS.md>).

### Find libraries and review prompt edits on the phone

In Android **Sequence**, select Drums to find **FM1 groove bank**.
Musical sequence starters remain available directly on the FM-1. Connect the phone directly to the FM-1
and stop playback before browsing/applying. The banks come from firmware rather
than duplicated app presets. Native listen/apply and replacement undo remain
available on the instrument.

Tap **Describe a sequence · offline recipes** to edit existing notes with a
supported command. Choose the channel scope and optional note locks, then press
**Propose**. Review exact before/after changes before **Apply locally**, or
**Discard** to leave the pattern untouched. The example transposes the existing
channel-1 C note up one semitone; timing, length and velocity stay unchanged.

| Enter a supported edit | Review before applying |
| --- | --- |
| <img src="../../assets/screens/merthsoft/guide/android-prompt-command.png" width="280" alt="Offline transpose prompt with scope and note-lock options"> | <img src="../../assets/screens/merthsoft/guide/android-prompt-review.png" width="280" alt="Review changes pitch 60 to 61 with Discard and Apply locally actions"> |

Prompts use bounded offline edit rules. **Compose a new offline loop…** in the
same dialog generates a separate editable draft. Neither workflow generates WAV
audio; the connected instrument supplies sound. The pictured proposal was
discarded after capture.

Packed one-bit fonts keep the refreshed bold lettering compact. Factory drum
sounds share losslessly identical parameter bases, with each sound’s exact level
retained separately. Only note-on expands a factory record; audio rendering reads
a stable record belonging to that voice. Six 22-byte records add 132 bytes of RAM,
while the bank representation saves 5,158 bytes before decoder code. Editable SYN
kits, downloaded sounds and saved projects retain their existing expanded formats.
Startup uses a firmware-title screen instead of the bitmap splash. No additional
full-pattern buffer is needed for ROM starter libraries or modulation locks.

Native images in this guide are fresh **production-code framebuffer
renders**; Android images are fresh captures from the connected Pixel. Both were
visually reviewed October 10, 2026. The native renders use host panel/storage ports,
not photographs or redesigned mockups. No hardware scale-light photo is included.
Build/host regressions are recorded in the
[verification log](<../../android/design docs/VERIFICATION.md>); they do not measure
physical hardware audio deadlines or remaining stack margin.

### Mix any starter progression with any rhythm

Turn **PRESET** in the SEQUENCES browser to select RHY. ORIGINAL preserves the
starter behavior; 17 explicit masks apply to any progression in chord, bass or
arp-note mode. The rhythm choice persists across tracks, with shared Preview/Apply.
FEEL is a uniform early/late nudge; it does not randomize or swing notes.

![Independent rhythm selection](../../assets/screens/merthsoft/guide/feature-starter-rhythm-choice.png)

## Layer sequences and retain the effects you set

In SEQUENCES, turn ALGORITHM to ADD instead of REPLACE before applying. ADD keeps
track timing, locks/fill and existing dynamics; it skips duplicate pitches and
rejects overlaps over four distinct notes without changing the track. Ties from
both layers are resolved, with shared whole-chord attacks when either changes.
Preview plays the starter alone; EDIT undo/redo restores the complete change.
The phone's existing Apply remains a replacement.

Changing a track's instrument preserves its voice mode (POLY/MONO/LEGATO/UNISON),
glide amount/mode, note priority/allocation, drive, chorus, delay/reverb sends,
insert, slicer and filter. On a normal FX-family page, press OCT− and OCT+ together
to reset only effects on the selected track; playing preferences stay intact. Song loading still recalls
the saved effects; INIT SOUND remains an explicit complete sound reset.

## Track auto-wah and auto-pan

Hold **ENV+FX** for auto-wah or **LFO+FX** for auto-pan; physical keys still play and
scale lighting stays intact. Wah knobs are rate/sensitivity, depth, base center,
and source (free LFO, beat division or ENVELOPE). Pan knobs are rate, depth,
waveform and beat sync. Hold the combo and tap HOME to lock the effect; HOME,
STOP/panic or track changes clear it. Saved patch parameters stay intact.
See [the full controls](AUTO-WAH-DESIGN.md) and [branding/version policy](../BRANDING.md).

| Track envelope wah | Track auto-pan |
| --- | --- |
| <img src="../../assets/screens/merthsoft/guide/auto-wah-envelope.png" width="240" alt="MerthSloop signal-following auto-wah with sensitivity and cutoff controls"> | <img src="../../assets/screens/merthsoft/guide/auto-pan.png" width="240" alt="MerthSloop track auto-pan with waveform and beat sync"> |

*Fresh production UI/audio harness renders.*

## Individual notes, action sheets and drum FX

On **SEQ → NOTES**, knob 1 walks chord members/empty cells, knob 2 edits the selected chain's pitch, knob 3 its length and knob 4 its GHOST/SOFT/NORM/HARD level. PRESETS zooms STEP and NOTES to 8/16/32/64 steps. Tied notes remain joined across later attacks. EDIT taps delete just that chain; undo remains EDIT+OCT−. Pitch, length, move, copy, delete and pattern undo/redo work during playback. Sounding attacks reconcile at the next step without cutting other voices or tracks; remaining ratchets on the edited track pause until that step. Structural controls require stopping/disarming an active recording or free take, which otherwise owns pattern writes.

**NOTE ACTIONS** has SLIDE STEP, NUDGE STEP, FILL STEP, ACCENT STEP, MOVE NOTE, COPY NOTE and DELETE NOTE. Knob 1 selects a row; knob 2 edits a shared field or stages a destination. OCT+ applies a discrete operation once, and OCT− returns to NOTES. Existing voices and shared step automation are retained. Collisions, full chords, pattern-end overflow and locked source onsets refuse without editing the song. A move may overlap its own sustain.

The drum pages add **HIT ACTIONS** after RHYTHM. Its selected lane/step are pinned on entry. Ratchet is per hit; nudge/fill say STEP. Move/copy/delete retain other lanes and expression. OCT− returns to GRID. Physical action-sheet keys are silent, function layers retain priority and PLAY/REC close the sheet normally.

| NOTES | NOTE ACTIONS | HIT ACTIONS |
| --- | --- | --- |
| <img src="../../assets/screens/merthsoft/neon/notes.png" width="240" alt="Selected sustained chord member in NOTES"> | <img src="../../assets/screens/merthsoft/neon/note-actions.png" width="240" alt="Selected-note actions with staged move destination"> | <img src="../../assets/screens/merthsoft/neon/hit-actions.png" width="240" alt="Drum hit actions retaining lane and source step"> |

The drum track's first **FX** page is LEVEL / PAN / DLY / REV, with real drum sends shown as percentages. Its second page is the track filter. **INSERT 2** exposes MIX / TYPE / the first two sound controls on both drums and synths.

| Drum FX | Drum filter | Insert blend |
| --- | --- | --- |
| <img src="../../assets/screens/merthsoft/neon/drum-fx.png" width="240" alt="Working drum delay and reverb sends"> | <img src="../../assets/screens/merthsoft/neon/drum-filter.png" width="240" alt="Native drum track low-pass filter"> | <img src="../../assets/screens/merthsoft/neon/insert-blend.png" width="240" alt="Insert blend page with four useful controls"> |

## Master LOFI characters

**FX → LOFI** selects OFF, VINYL, TAPE, VHS, CD, RADIO or SMPLR. Their three controls are WOW/DARK/DAMAGE, WOW/DARK/WEAR, WARBLE/DARK/DAMAGE, RATE/LATCH/MIX, and DRIFT/BAND/STATIC. Vinyl uses irregular pitch drift and sparse varied clicks/pops; tape adds flutter and saturation; VHS adds faster warble/dropouts; CD repeats tiny captured fragments; RADIO adds a narrowed receiver band, drift and hiss/crackling static, with more audible low/mid control settings. CD is microstutter, not a phrase-length CSIDMAN delay. That longer delay remains a separate planned expansion.

All amounts zero bypass the processing. Characters and amounts save with sections/projects; older vinyl settings load as VINYL. The held LOFI punch intensifies the selected character, using Vinyl when TYPE is OFF, and restores the saved settings after release.

| Vinyl | Tape | VHS | CD |
| --- | --- | --- | --- |
| <img src="../../assets/screens/merthsoft/neon/lofi-vinyl.png" width="200" alt="Vinyl character controls"> | <img src="../../assets/screens/merthsoft/neon/lofi-tape.png" width="200" alt="Tape flutter, darkness and drive"> | <img src="../../assets/screens/merthsoft/neon/lofi-vhs.png" width="200" alt="VHS warble and damage"> | <img src="../../assets/screens/merthsoft/neon/lofi-cd.png" width="200" alt="CD short-fragment stutter controls"> |

MIDI controllers also gain **CC11 expression**, reset by CC121/project/section adoption, and **CC0 Bank Select** with Program Change. Bank 8 selects physical user-preset slots; empty slots are ignored. Saved levels and playing/effect preferences stay intact, and stale queued Program Changes cannot overwrite newly loaded sounds. See [MIDI details](MIDI-EXPRESSION-BANKS.md).

### Piano-roll grid and LOFI bypass

STEP and NOTES include shaded semitone rows, octave labels and beat/bar guides.
A white outline marks the selected independent note; the playhead spans the grid.
PRESETS turns left to zoom in and right to zoom out through 8/16/32/64 steps.
The graph stays below the knob cards.

![Piano-roll grid with an independently sustained chord](../assets/screens/merthsoft/neon/notes.png)

LOFI TYPE starts with OFF, keeping all three amounts for re-enable. OFF saves with
projects and sections, and LOFI punch remains available as a temporary override.

![LOFI starts with OFF](../assets/screens/merthsoft/neon/lofi-off.png)

### Delay characters

**FX → DLY TYPE** selects DIGI/TAPE/CRUSH/DUB on knob 1. The remaining knobs are TIME/FDBK/MIX for DIGI, DRIVE/TONE/MIX for TAPE and DUB, and BITS/RATE/MIX for CRUSH. RATE spans about 2.8–44 kHz; higher values are cleaner. Original DLY keeps TIME/FDBK/COLR/MIX for every type.
DIGI preserves the original delay, TAPE compresses its repeats and CRUSH adds
coarse resolution/sample holding. DUB filters the first repeat and feedback with
a darker TONE range, stronger saturation and progressive bass trimming. DRIVE
responds earlier while retaining its full adjustment range. The preceding DLY page retains COLR. These
settings use the shared delay send bus and save with sections/projects.
Master LOFI remains separate and defaults to OFF. STEP/NOTES footers now
explicitly name PRESETS zoom.

![Tape delay type](../assets/screens/merthsoft/neon/delay-tape.png)

![Crush delay bit depth and rate](../assets/screens/merthsoft/neon/delay-crush.png)

| Radio LOFI | Dark reverb | Dub delay |
| --- | --- | --- |
| ![Radio](../assets/screens/merthsoft/neon/lofi-radio.png) | ![Dark](../assets/screens/merthsoft/neon/reverb-dark.png) | ![Dub](../assets/screens/merthsoft/neon/delay-dub.png) |

### Native chord inversions and spread

**SEL → VOICING** is the third synth harmony page: **INVERT / SPREAD / VLEAD /
STRUM**. INVERT rotates voices by octaves (-3..+3). CLOSE keeps their spacing;
OPEN lifts alternating voices; WIDE lowers the bass and raises the top voice.
For C4–E4–G4, OPEN gives C4–G4–E5 and WIDE gives C3–E4–G5.

Settings are independent per track, survive instrument changes, and save with
projects/sections. New projects reset them. Currently held/latched notes stay
unchanged until another chord or modifier revoice. The four-note chord and
shared eight-voice limits stay intact. See [implementation](CHORD-VOICING-DESIGN.md).

![Native voicing controls](../assets/screens/merthsoft/neon/chord-voicing.png)


### Scale-light brightness

Hold HOME, select **LIGHTS**, then turn **knob 4 — SCALE LEVEL** to choose
DIM / LOW / MID / HIGH. DIM matches the original scale-guide brightness.
Played-note LEDs retain full intensity. The level saves with device preferences;
close the menu and stop playback to allow any deferred settings write.
SEL+OCT+ remains the independent scale-light on/off toggle.

## Additional factory instruments

The appended CHIP bank adds NES PLS12, NES PLS50, NES PLUCK, NES TRI, GB WAVE BS, GB HOLLOW, GB ORGAN and GB ARP. INST adds VIOLIN, CLARINET, OBOE, BASSOON, HARPSICHORD and OUD. All fourteen use existing engines and normal sound/envelope controls; save edited variations to USER. These are synthesized interpretations, rather than exact console emulation or orchestral samples. See the [preset pack design and validation](INSTRUMENT-PRESET-PACK.md).

## Recording and sound-browsing refinements

Live POLY recording preserves overlapping notes until their individual releases.
Incoming legato records the voice mode's non-POLY handoff, with independent ties
remaining available for guitar-style sustain. Editing and project round-trips
retain the marker. An untouched factory/user arp follows its preset; changing
its arp values makes it yours to keep through sound changes. HOME displays the
actually running A–D arrangement section. See the [integration scope](UPSTREAM-2.8-INTEGRATION.md).

![HOME showing the running section C](../../assets/screens/merthsoft/neon/home-playing-section.png)

### More LOFI sounds to try

On **FX → LOFI**, choose **TAPE** for worn tape: **WOW / DARK / WEAR**.
Choose **SMPLR** for early-digital crunch: **RATE / CRUSH / DRIVE**. Higher RATE
amount means a lower sampling rate; higher CRUSH means fewer bits. DRIVE is
level-compensated clipping. Each control blends progressively: low settings stay gentle, while maximum settings retain heavy destruction. Start around 25–40% per control and increase to taste.
All three zero bypasses the effect; OFF remains first and retains saved amounts.

Near the end of PRESETS, the **LOFI** bank adds **DUST KEYS, WORN PAD, TOY BELL
and DUST BASS**. These are editable instrument sounds and leave the master LOFI
choice alone. Combine the warm presets with TAPE, or try SMPLR on drums.

### Slow ambient motion and sustained starters

**LFO → DRIFT** offers MODE (OFF / DRIFT / DRONE / BOTH), CYCLE (5–160 seconds), AMOUNT and TARGET
(TONE / SHAPE / PAN). Each synth track has independent smooth random motion,
with fading amount/OFF/target changes, a live bipolar motion meter and cycle
progress. Full amount spans roughly ±64 control units; LOFI TONE responds in
both directions. Fresh defaults are OFF, 10S, about 50% amount. It layers over the edited sound and
regular LFO without changing their parameters. These runtime controls reset
at power-on and are unavailable on drums.

![Native DRIFT controls](../../assets/screens/merthsoft/neon/ambient-drift.png)

**STILL WATER**, **WIDE SKY** and **DUSK** append sustained progressions to
SEQUENCES. Try CHORD, VLEAD and a slow attack/release; existing key, rhythm,
preview and layer controls still apply. DRONE/BOTH adds tap-toggle pitches without retriggering the other held notes.
Selected-note lights follow octave changes; track changes preserve the notes.
Use POLY and a sustaining envelope for a chord bed. STOP clears all holds;
MIDI/sequence releases cannot cut a pitch still owned by DRONE. DRONE entry
takes over live ARP/chord input without stopping transport. Mode and notes
reset at power-off.

![Native DRONE mode](../../assets/screens/merthsoft/neon/ambient-drone.png)
 See [scope and validation](AMBIENT-PERFORMANCE-DESIGN.md).

### Slow drone-note evolution

**LFO → EVOLVE** replaces at most one drone pitch per timed or tempo-synced cycle,
with a live cycle bar. Seconds range 1–160; synced choices span sixteenth-note
triplets to 16 bars and pause with transport. Following ROOT/SCALE, CHANCE controls how often; RANGE allows 1–7
scale steps per change. Other held voices do not retrigger. OFF keeps the current
chord; STOP clears it. Changes skip duplicates, MIDI limits and pitches shared
with other producers. Use DRONE/BOTH, POLY and a sustaining envelope. Normal REC
captures sounded notes; otherwise stored patterns and patches are untouched.
Runtime-only, with no new saved-format fields. Try 5S / 100% / RANGE 1 first.

![EVOLVE controls](../../assets/screens/merthsoft/neon/ambient-evolve.png)

### Track sustain

**ENV → SUSTAIN** offers OFF/PEDAL/PIANO/STRINGS/LOFI, DECAY 0.5S–32S/HOLD, DAMP and AMOUNT. Released notes
can ring beneath new POLY attacks, retaining their engine/envelope behavior with
an independent tail fade. Logical gates, MIDI releases, recording and independent
ties stay intact; tails yield voices before held notes. OFF hands tails to normal
release, STOP clears tail ownership. Runtime-only; drums excluded. Naturally
ending sources still end. PIANO fades treble faster and darkens more; STRINGS darkens gently; LOFI reduces sample rate/resolution of released tails. New attacks stay clean. DAMP controls darkening and LOFI coarseness; AMOUNT sets extension level, zero restores ordinary release. Settings reset at power-off.

![Sustain foundation](../../assets/screens/merthsoft/neon/track-sustain.png)
