# Eleven distinct chord modifiers

Status: implemented in native firmware. Eleven physical black keys now have distinct
functions. The four-note voice budget, project layouts and chord-shape IDs remain
unchanged. Recorded chords/arp output use the existing literal-note recorder.

Use the physical printed black-key positions, independent of ROOT/scale/playing
octave. Keep the first five familiar gestures; use the upper six for new functions.

| Physical key | Modifier |
| --- | --- |
| F#3 | Flip major/minor third |
| G#3 | Add diatonic seventh (existing behavior) |
| A#3 | Sus4 |
| C#4 | Add ninth |
| D#4 | Invert upward |
| F#4 | Sus2 |
| G#4 | Add thirteenth |
| A#4 | Explicit major seventh |
| C#5 | Explicit minor seventh |
| D#5 | Invert downward |
| F#5 | Open voicing |

Momentary holds and chord-latch toggles must both work, including changing a
retained arp/chord without reapplying the root. CHROM keeps all keys as roots.
Progression LIVE currently owns white pads and suppresses normal modifiers;
sharing its chord builder remains separate future work.

## Combination rules

- Preserve the four-note voice budget; adding an extension may replace a fifth.
  Root and third/suspension remain. A requested seventh takes the next slot;
  if both ninth and thirteenth cannot fit, thirteenth takes the last slot.
- Sus2/sus4 form a mutually exclusive latch group. With both physically held,
  choose documented sus4 precedence; release restores the remaining modifier.
- Explicit major/minor sevenths override the diatonic-seventh modifier. They form
  a mutually exclusive latch group; simultaneous momentary holds prefer minor7.
- Up/down inversion cancel when both are held. Apply manual register and open
  voicing after automatic voice leading so it cannot cancel the explicit gesture.
- Scale-aware shapes derive 9/13 from scale degrees; fixed-quality shapes use
  their documented semitone extensions. Deduplicate and respect MIDI bounds.
- POWER/OCTAVE keep existing pitch-quality limitations; inversion/open voicing
  can reshape them without silently introducing a third.
- Replace the five-function guide with an eleven-position layout. Distinguish
  physical positions from sounding pitches after octave transposition.

## Implementation and validation

The eleven-bit modifier set and per-synth latched flags use a 16-bit word.
Momentary modifiers are filtered by their owning track. Existing root/arp revoicing
reuses the same bounded chord builder; no additional voices, audio buffers or timer
are introduced. Register gestures are applied after automatic voicing and excluded
from its reference history to prevent repeated modifier edits drifting octaves.

A single extension insertion helper preserves sorted input between extensions,
replaces a fifth before sacrificing another outer tone, and refuses out-of-range
notes. Final pitches are sorted and deduplicated. The key table stores one small
index per physical key instead of repeating pitch-class handlers. CHROM displays
the normal scale guide, since it has no modifier keys.

`tests/chord_modifiers_test.c` checks each physical key, held and latched direct/arp
ownership, balanced raw/generated MIDI, exclusive quality toggles, track isolation,
CHROM roots, extension precedence, repeated voice-leading stability and every
2,048 modifier combination across all chord shapes/scales and six MIDI registers.
The exhaustive matrix covers 3,932,160 chord outputs with four-voice buffer sentinels.
Existing chord/scale/arp/voicing/storage, audio and UI regressions remain required.

The final target image is 584,336/584,336 bytes (0 free), with unchanged linked RAM:
main 89,128/98,304, low 28,280/29,496, audio pool 332,876/344,064. Relative to the
preceding build this adds eight image bytes; the unified quality/extension and
register-shaping code replaces the old duplicated five-modifier paths.
