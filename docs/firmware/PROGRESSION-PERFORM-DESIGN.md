# Progression performance on the FM-1

Status: integrated native firmware feature. Direct sustained chords (ARP OFF) and existing arpeggiator
rhythms (ARP ON) share immediate chord selection. Starter-rhythm playback and quantized chord-change
queues remain deferred; they are not selectable controls in this build.

## Player workflow

1. On a synth track, tap ARP and select OFF for sustained direct chords, or enable a
   mode for rhythmic output. UP is a useful starting point; PULSE repeats the chord. Set RATE and GATE there as desired.
2. Use loop playback rather than SONG mode; transport can keep running.
3. Open SEQ's SEQUENCES browser. Choose a stock progression or edit CUSTOM,
   its ROOT/SCALE, and optional VLEAD on the pitch page.
4. Turn SELECT to the fifth browser page, PERFORM. Turn knob 1 LIVE to ON.
5. White keys from the left select progression positions in order. For a
   5-1-2-4 draft, the first four white keys select those four chords. The next
   group repeats them one octave higher. Groups repeat through all sixteen white
   keys: a three-slot draft has five complete groups plus its first chord five octaves up.
6. Turn knob 3 HOLD on to keep the last chord after releasing its pad.
   PLAY runs the other tracks alongside it. REC captures sounded chords or arp
   attacks into ordinary editable sequencer notes.
7. Turn LIVE off or STOP to end live ownership. Navigate to ARP, SEL or FX to adjust the sound while it continues.

PERFORM knobs: **LIVE / OCTAVE / HOLD / SLOT**. SLOT displays the current position;
its knob does not alter the progression. OCTAVE affects the next selected chord,
including when HOLD is on, without changing the chord already playing. The screen
states RHYTHM: ARP / NOW, or OUTPUT: CHORD / NOW with ARP OFF. Its chord cards show the draft degrees and highlight
the selected position. The ordinary starter RHY/rotate/offset/feel settings do not
control this mode: with ARP ON its rhythm is the track's ARP settings. ARP OFF sustains until release/HOLD change or another pad.

## Shipped harmonic scope

Stock progressions use their diatonic triad/seventh flag; CUSTOM uses triads.
Root, scale and octave come from the sequence-browser draft, matching Apply's
harmonic material. The standalone SEL CHORD shape does not override this first slice.
All eleven black keys use the same modifiers as ordinary SEL chord mode: quality
toggle, seventh, sus4, ninth, inversion up, sus2, thirteenth, major seventh, minor
seventh, inversion down and open voicing, from left to right. These act on the
browser's triad/seventh material and its ROOT/SCALE, including when SEL CHORD is
OFF or its keyboard mapping is CHROM. With HOLD off they are momentary; with HOLD
on each press toggles its modifier until pressed again. Sus2/sus4 and major/minor
seventh alternatives are mutually exclusive. A modifier alone does not start a chord.
The PERFORM page shows **BLACK: MODIFIERS** beside its white-slot footer hint.
Modifier changes rebuild the sounding slot's captured degree, root, scale and
octave; editing those draft knobs still affects only the next white-pad press.
The shared chord modifier policy is called directly, without temporarily changing
track parameters or copying a second chord builder.

Shared inversion/spread controls from SEL VOICING shape the result without adding
voices. Sequence-browser VLEAD reuses harmony_voice_lead. Its history stays at the
reference register before explicit octave offset and manual voicing: repeated upper
pads retain their requested octave rather than cancelling it through nearest-note
search. MIDI-range clipping deduplicates pitches at the extremes.

One slot owns the selected track at a time. Last newly pressed pad wins; releasing
an older pad cannot release its replacement. Pressing the same slot again resets
pitch traversal without restarting the arp timing or transport. Selection releases
the prior generated gate, then the existing arp grid controls the next attack.
With HOLD off, releasing the owning pad ends its source and generated notes.
Turning HOLD off after the pad is released also ends it.
Modifier changes preserve unchanged sustained pitches and running arp gates,
including octave-expanded notes and UP8's extra tonic. Retired pitches end; direct
output starts only newly added pitches, while ARP waits for its next normal grid
attack. Modifier changes do not reset arp timing or its traversal position.

## Ownership and lifecycle

The small runtime descriptor owns one synth track, using its existing held[] arp
source. Pitches are literal (held_key zero), so keyboard scale mapping is never
applied a second time. No audio buffer, heap, extra voices or scheduler was added. Direct output uses
the same generated-note list, with held recording and explicit release rather
than arp gating.
The common generated-note emitter sends audio, MIDI and recording events. Direct
chords use held-note recording and end those holds on release/replacement. They
reuse the existing generated-note list; arp_tick bypasses their sustained gates.
Crossing ARP OFF/ON while live transfers the current literal harmony between sustained
chords and the existing arp grid. It does not disarm or replace the source pitches.

Arming works during loop playback and tempo recording/count-in. It retires the selected
track's old physical/MIDI/pedal/keyboard-hold owners, restores parameter locks, releases
stored sequence ownership, stops audition, and resets reference voicing history.
Other tracks and transport continue. Release old physical keys before selecting fresh pads.
Free-time capture and pending owner panic still prevent arming.

New MIDI note-ons directed at the live owner are ignored before source allocation;
other tracks continue to receive MIDI normally. Existing note-off/pedal/transport
cleanup remains serviced. Direct LIVE input cannot add another harmony source.
Routing changes, panic, preset/engine changes and track changes end ownership.

Browser draft knobs, PRESETS and ALGORITHM remain editable while live. Existing
held harmony is a literal snapshot: root, scale, degree, octave and voicing changes
apply on the next pad press. HOME and ordinary page navigation retain LIVE; returning
to the browser opens PERFORM. Held FX/SEL/ARP/ENV/LFO layers suspend new pad input
while the existing harmony continues. Project/section adoption, destructive REC hold
and STOP end ownership. Stopped-only audition and destructive starter Apply retain
their separate restrictions; draft edits do not require either operation.

## Stored playback and recording

The owner's stored pattern is temporarily silent, byte-for-byte unchanged unless
recording is enabled. Grid/pass and recorder bookkeeping continue; stored note,
lock, microtiming, ratchet and SNOTE producers are bypassed. Other tracks retain
normal timing. Disarm adopts the current transport token and current SNOTE route,
so stored playback resumes at its next event without replaying an elapsed step.

REC records sounded output once: generated arp attacks, or held direct chords
with normal recorded ties. It never stores semantic progression commands.
Stopped entry uses the existing TEMPO recorder, even when the user's normal empty
project preference is FREE. That preference is not changed; the record screen shows
TEMPO for this mode. START NOTE and PLAY/count-in work normally. A count-in does not
record monitored notes before the downbeat. Existing quantization, velocity, undo,
overwrite/overdub and four-notes-per-step limits apply, including expanded PULSE.
Live state itself is ephemeral. Recorded ordinary notes save through normal songs;
no project format, upstream parameter IDs or preset layout change is introduced.
Direct modifier recording carries retained pitches as continuation-flagged notes
when a new onset step is needed, and terminates only retired continuation chains.
This uses the existing note-chain and recording helpers; unrelated overdub notes
and later deliberate reattacks remain intact.
Ahead-of-grid recording skips are unique pitches, matching the existing four-note
step capacity. Repeated modifier edits within one target step cannot consume that
budget with duplicate retained pitches or produce a second boundary attack.

## Validation and measured cost

Focused actual-engine/UI tests cover custom 5-1-2-4 in C minor, last-pad ownership,
all sixteen white pads with three-slot groups, all eleven momentary/toggled
modifiers, counts 1-8 with extreme registers and sparse-scale voice leading,
unchanged UP/UP8/REPEAT/PULSE gates, exact modifier recording/continuation metadata,
ordinary playback of those recorded chords, near-boundary skip capacity and loop wrap,
HOLD, octave-repeat and shared voicing, MIDI/pedal/physical-source takeover, other-track
input, stopped-source PLAY, stored-pattern preservation/SNOTE isolation, natural-grid
resume, held-layer suspension, running entry, live draft/page navigation, direct/arp transfer, generated recording, count-in and cleanup. Optimized and UBSan runs pass, including 300 deterministic pad/HOLD/
octave transitions with balanced MIDI. Punch keys remain usable over the live arp.
All 182 ordinary audio golden renders are unchanged with LIVE off. Existing
chord/arp, canonical upstream project, real section/autosave and UI fuzz regressions
remain applicable. Target flash/RAM and placement checks must pass before packaging.

The private development experiment under build/progression-perform-prototype was
only feasibility evidence. Its 1,272-byte measurement did not include this final UI
and lifecycle. The integrated image measurement is recorded in OPTIMIZATION.md;
never present that private kernel's size as the finished feature cost.

## Deferred extensions

- NOW/BEAT/BAR chord-change queues: consume the actual transport clock, including
  external sync, commit before same-boundary arp attacks, and cancel released
  requests when HOLD is off. Do not add another wall-clock rhythm timer.
- Starter-rhythm output: extract shared attack masks and rhythm shaping from Apply,
  drive them from the transport's swung grid, and reuse the normal note/MIDI/recorder
  path. The preview engine is intentionally not a performance scheduler.
- Alternative base chord shapes beyond the browser's triad/seventh material.
- Concurrent MIDI harmony and persistent performance drafts require explicit source
  identity and storage design; neither is implied by current local-pad ownership.

Hardware feel remains user validation after installing the served build.

The integrated **2.7.1.52** target is **582,048 / 584,336 B**, leaving **2,288 B**
flash. Main static RAM is **86,144 / 98,304 B**; low RAM and the audio pool retain
their previous allocations. This includes the upstream-controls batch and review
fixes too; it is not a standalone size estimate for progression performance.
