# Track sustain

Status: PEDAL / PIANO / STRINGS / LOFI, DAMP and AMOUNT implemented and host-validated. Persistence and hardware listening/deadline qualification remain open.

## Musical purpose

Let released notes ring beneath later attacks, complementing independent note
ties, drone holding and EVOLVE. This is a track-local voice-tail processor, not
room reflections or a held reverb buffer. It does not replace existing ADSR,
MIDI CC64, ties, legato or DRONE ownership.

The ENV-family SUSTAIN page has four controls:

| Control | Meaning |
| --- | --- |
| TYPE | OFF / PEDAL / PIANO / STRINGS / LOFI |
| DECAY | Tail duration; PEDAL includes an indefinite end position. |
| DAMP | How quickly the ringing tail darkens; per tail, never the whole track. |
| AMOUNT | Strength of the extension, with zero preserving ordinary release. |

Names are musical behavior contracts, not preset aliases for a longer REL:

- PEDAL lets released voices continue their held behavior, then die according to
  the sustain decay. Releasing/turning off the pedal returns tails to ordinary
  release without interrupting physically held, MIDI-owned or drone-owned notes.
- PIANO gives released notes a pitched decay with faster treble decay and gradual
  high-frequency damping. No claim of sympathetic-string/acoustic piano modeling.
- STRINGS gives a smoother ringing tail with gentler damping and longer decay,
  suitable for guitar voicings that overlap later attacks.
- LOFI must actually degrade the tail (bandwidth, stepped amplitude or similar),
  preserving clean new attacks. It must not merely turn up the master LOFI effect.
  Its particular processing and engine coverage require a measured prototype.

The complete character slice uses the same bounded voice lifetime as PEDAL. No second synth voice or feedback/history buffer is created.

## Existing mechanisms and engineering boundary

The ordinary shared envelope currently moves to release on trk_note_off.
However, simply extending P_REL is insufficient: FM6 has its own gate-sensitive
operator release; held-loop samples change looping on gate-up; CHIP STEP has its
own amplitude envelope. Physical models and unlooped samples can decay/end even
with a held gate. Sustain cannot recreate energy or audio already gone.

Separate logical note ownership from engine sustain for a released tail. A tail
is NOT a held physical/MIDI/sequence/drone owner. Allocation and overload shedding
must rank it with released voices, preserving weighted shared admission and engine caps. Use bounded per-voice metadata if necessary, never an
unbounded delayed-note queue. Keep audio state audio-owned.

Sustain begins only when the last real owner of a pitch releases it. Engine gate,
ADSR tail and per-voice tone modulation must agree. Repeated pitches deliberately
reattack using existing allocation; old scheduled release events cannot kill a
new attack. Independent continuations do not reattack or begin a sustain tail.
DRONE and EVOLVE continue to use their existing captured pitches/channel rules.

Physical/MIDI/recorded inputs should get the same local audible tail behavior.
Normal recording still stores the played gate duration, not extension as a tie
chain; a later explicit bake would be separate. Outgoing MIDI note ownership stays
balanced and unchanged by this local sound effect. Existing incoming CC64 remains
an independent real pedal owner; its release must not cut notes another source owns.

## Lifecycle and persistence

TYPE/AMOUNT changes should slew where audible; turning OFF releases only the
processor's existing tails through normal release. STOP, panic, preset/engine
adoption, project adoption and route cleanup must leave no retained ownership.
Do not leave a flag attached to a reused voice slot. Knob edits must not overwrite
ADSR, filter or master LOFI values.

A measured runtime-only build is the first validation slice. Persistent controls
are an explicit follow-up: retain upstream project compatibility and do not grab
unused-looking upstream P_/G_ IDs. Explain unsaved controls clearly until a
compatible storage design exists. Track changes should not silently reset the
running effect on another track.

## Acceptance and budget

Current pre-feature build: 2.8.0.67, 582,964 / 584,336 B, 1,372 B flash free;
main static RAM 89,456 / 98,304 B, low RAM 28,928 / 29,496 B. These are budgets,
not an estimate that every mode will fit. Measure the core before extending it.
No runtime factory-sample decompression or reduced instrument/voice counts.

Require default-OFF unchanged audio goldens; pedal release while keys remain down;
physical/MIDI/sequence/drone overlaps; guitar-style overlapping independent ties;
repeated pitches under long tails; bounded allocator/overload behavior; sample
loop/end behavior, FM6 operators, CHIP envelopes and physical-model behavior;
quiet-tail termination; track/preset/route/STOP cleanup; knob isolation and redraw;
per-type audio comparisons across useful low/mid/high settings; target flash/RAM,
worst-case polyphony/CPU and native/WASM parity. Confirm audible differences and
hardware deadlines before treating the characters as complete.

## Implemented PEDAL foundation

Open **ENV → SUSTAIN**. Knob 1 TYPE = OFF / PEDAL. Knob 2 DECAY = 0.5S,
1S, 2S, 4S, 8S, 16S, 32S or HOLD. Defaults are OFF / 8S. Knobs 3/4 are
inactive: no unimplemented sound characters, damping or amount controls appear.
Use POLY for overlapping notes; enabling PEDAL while playing applies when those
notes are released. It does not resurrect an already released/dead note.

On actual release the logical gate goes up immediately (v.gate = 0), so MIDI
note-off timing, independent ties, allocator priority and recorded gate lengths
stay intact. A separate runtime flag keeps the engine's held behavior alive,
while the original envelope continues its attack/decay/sustain. A high-precision
Q15.9 amplitude factor fades the released voice. Decay names are approximate
RT60 times; HOLD adds no tail attenuation. The source may still decay or end:
this does not create new sample data, energy, resonances or extra voices.

FM6 operator gates, held sample loops, WHEEL release clicks and optional SLICE
loop gating use the common engine-held accessor. Logical ownership checks still
use the original gate. Retriggering the same pitch clears old tail state. Released
tails remain preferred steal/overload victims under the existing shared budget.
OFF hands only tail-owned voices to ordinary release at their current gain.
STOP retires all acoustic tails after the normal source releases; physically held
keys are not cut. Full panic/all-off and engine adoption clear retained flags.
Track controls remain runtime-only; browsing tracks preserves their settings.
The upstream project layout and saved parameter IDs are unchanged.

### Target and verification

The combined .68, including tempo-synced EVOLVE and the audited build-only LZ4
parser improvement, is **584,064 / 584,336 B**: **272 B flash free**. Main
static RAM is **89,616 / 98,304 B** (8,688 B free), low RAM **28,928 / 29,496 B**
and pool **331,852 / 344,064 B**. Net change versus .67 is +1,100 B flash and
+160 B main RAM; do not attribute the entire combined delta to sustain alone.
No audio buffer, instrument/preset data or runtime decoder is added.

Tests cover all finite decay durations within 10% of nominal RT60, indefinite
HOLD, OFF preserving physical gates, repeated-pitch attacks, release-first voice
stealing, incoming pedal/drone ownership, independent ties across new onsets,
physical-key MIDI on/off packets while the local tail rings, STOP cleanup,
engine gate handling, per-track native knobs and drum exclusion. Sanitized
multi-track tests exercise real second-core jobs with delayed scheduling. All
200 default-off audio goldens remain unchanged. UI, target and browser-parity
checks cover the combined build. Hardware listening/deadline/stack testing remains
pending. The subsequent .69 integrates cold sharing and weighted admission,
including FM6 eight, at 583,776 B (560 B free), with unchanged static RAM.
Physical slots remain eight per track; cheap LOFI/WHEEL parts can share twelve
voices. Steal-fade retriggers must reacquire budget before sounding again.

![Native sustain foundation](../../assets/screens/merthsoft/neon/track-sustain.png)

## Implemented character extension

TYPE now includes PIANO / STRINGS / LOFI. Knob 3 DAMP and knob 4 AMOUNT are active; defaults remain OFF / 8S / 0% / 100%. PIANO scales decay by `(64 + MIDI note) / 128`, giving faster treble loss, and stronger low-pass darkening. STRINGS keeps the selected decay with gentler darkening. LOFI uses a per-tail sample hold (2/4/8/16 samples as DAMP rises), reduced resolution and darkening. PEDAL remains unchanged at DAMP 0 / AMOUNT 100%. DAMP includes an initial coloration even at HOLD, then strengthens as gain decays. All modes retain clean held attacks. These are tail characters, not physical/sympathetic string models.

AMOUNT scales extension level after release, using the existing control-block amplitude ramp. Zero starts ordinary release; when turned to zero or OFF during a tail, its last extension gain is retained through ordinary release. Repeated notes reset tone state; no engine state slots are reused. Each voice has two independent tone values and initialization/amount metadata. Character rendering uses a 128-byte stack scratch block, no persistent audio/history buffer. Each track render owns its own scratch and voice state on its assigned core.

The .70 combined target is **584,204 / 584,336 B**, leaving **132 B flash free**. Relative to .69: +428 B flash and +320 B main static RAM (89,936 / 98,304 B, 8,368 B free). Low RAM 28,928 / 29,496 B and pool 331,852 / 344,064 B are unchanged. Do not treat static RAM as measured stack margin.

Maintained tests compare complete held-note audio across all modes, distinct tail hashes, positive extension, lower AMOUNT energy, DAMP spectral/energy reduction, pitch-dependent piano decay, HOLD/zero lifecycle, and character rendering across all engines under UBSan. Multi-part/two-core sustain tests include all four characters. Default-OFF golden renders, UI controls, MIDI/tie ownership, weighted admission and browser/native parity remain required. Hardware listening, worst-case deadlines and stack high-water remain pending. Runtime-only: no new saved IDs or project layout.
