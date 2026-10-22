# Native chord voicing

Status: implemented in the working firmware; progression performance remains
a separate [design](PROGRESSION-PERFORM-DESIGN.md).

## Playing controls

On a synth track, tap SEL and turn SELECT to **VOICING**, the third page.

| Knob | Control | Behavior |
| --- | --- | --- |
| 1 | INVERT | -3..+3 octave rotations; zero keeps the existing inversion |
| 2 | SPREAD | CLOSE, OPEN, WIDE |
| 3 | VLEAD | Existing nearest-voicing policy |
| 4 | STRUM | Existing signed strum timing |

Positive inversion moves the lowest voice above the highest. Negative inversion
moves the highest below the lowest. Each move uses octaves and avoids duplicate
pitches even for chords already spanning more than an octave.

- CLOSE keeps the voiced chord's spacing.
- OPEN raises alternating voices an octave, skipping occupied octave pitches.
  C4–E4–G4 becomes C4–G4–E5.
- WIDE lowers the bass and raises the top voice an octave.
  C4–E4–G4 becomes C3–E4–G5.

These spread existing voices rather than adding voices. The four-note chord
limit and shared eight-voice allocator are unchanged. At MIDI boundaries an
impossible movement is refused; notes are never dropped or clamped together.

The settings affect new chords from the physical chord keyboard, including
chords fed to its arpeggiator or recorded through existing entry/record routes.
Literal sequencer notes, ordinary MIDI pitches and existing starter recipes are
not rewritten. A control turn leaves currently held/latched pitches sounding;
the new voicing applies on the next chord or an explicit modifier revoice.

Modifiers and automatic voice leading resolve first; manual inversion/spread
apply afterward. Automatic history retains the unshaped voicing so repeatedly
playing one chord cannot keep climbing by the requested inversion. Existing
modifier inversion remains separate and combines with the new control.

## Track ownership and persistence

Each synth track has independent controls, initially zero/CLOSE. Factory and
user-preset browsing preserve them. User presets do not capture them: they are
track harmony preferences, not engine sound parameters. New projects reset
them. Full projects, autosave/load undo and song sections retain them through
the existing project codec. Untagged files reset stale runtime settings to zero.
The drum track does not expose this page.

Runtime fields live outside `p[]`, preserving every upstream parameter ID and
user-preset record. FUN6 stays the same size. An optional signed G_NEWPRJ tag
encodes three five-bit voicing codes and a presence bit; neutral captures omit
it. Older upstream readers clamp it to zero, do not invoke NEW, and lose only
this extension on re-save. See [storage contract](CHORD-VOICING-STORAGE.md).

## Validation and size

The first real target link adds **544 application bytes** and **16 main static
RAM bytes** over the packaged .41 baseline: **580,604/584,336 bytes**, leaving
**3,732 free**. Main RAM is **89,096/98,304**, low RAM **28,280/29,496**, pool
**332,876/344,064**. No new audio buffer, voice budget or per-sample operation.

Pure-helper tests cover 1,050,000 randomized settings, exact chord examples,
canaries, hostile controls, duplicate/range/class preservation and UBSan.
Native UI tests cover actual knobs, track/preset retention, held/latch pitch
preservation, repeated voice-leading stability and new-project reset. Storage
tests exercise every valid setting, full/section adoption, invalid tags and
coexistence with all other saved effect metadata. Upstream fixture, starter
hash, keyboard/arp regression and default audio goldens are acceptance gates.
Hardware audition is not claimed.

The same release also changes the punch label to LOFI. With that additional
change the final image is **580,652 bytes**, leaving **3,684 free**; RAM is
unchanged from the voicing-only measurement. LOFI routing adds 96 bytes, and sharing the existing chord sorter recovers
48 bytes. The combined net increment is 48 bytes over the first voicing build.
