# MIDI file import

Implemented in Sequence → **Import MIDI file…**. Uses original C# code in Workstation;
no GPL web-editor implementation is copied. Existing Android dependency obligations
remain as documented in the licensing notices.

## Workflow

1. Choose a `.mid` or `.midi` document. Content is validated, regardless of extension.
2. Choose a source part (file track plus MIDI channel) and destination Synth 1–3 or Drums.
3. Preview through the connected MIDI destination; Stop or dismiss releases playback.
4. **Import into app** replaces that destination's canonical app MIDI channel (0/1/2/9),
   retaining other channels, source note timing/velocity and original PPQ timing after
   rescaling. Pattern length expands to complete 4/4 bars as needed. Existing locks
   block replacement. One sequence undo restores previous notes; tempo is set separately
   and is not part of note undo. Generic preview respects the selected output routing.
5. Alternatively **Review FM1 transfer…** reads fresh native patterns and constructs a
   conversion without writing. Review loss counts, audition the converted material, then
   explicitly **Replace FM1 track · verify**. The ordinary HardwarePatterns adapter
   checks the baseline again before writes and verifies readback. Save on the instrument
   for persistence. Existing independently edited local native snapshots remain intact;
   Read native patterns again before further hardware edits.

## Native conversion

The current DIV supplies the grid, including triplets and long divisions; active length
supplies the boundary. Length, DIV, tempo, sounds, locks, other tracks and dormant steps
remain unchanged. Active steps are replaced, resetting their micro timing, fill flags,
ratchets and slide/accent flags. Existing locks can still affect playback.

Onsets and endings round to the nearest step. Synth onsets merge duplicate pitches,
retain the four strongest unique pitches, use their maximum velocity, and tie through
their longest duration until the next onset. Unequal voice durations cannot be retained
individually. Drums map GM pitches into the firmware's sixteen lanes and four hit levels;
colliding hits retain the strongest velocity. Converted preview reflects these rules.
Reports count notes past the boundary, substantial onset shifts, duplicates, excessive
chord notes, shortened overlapping chords and adjusted duration groups. No-fitting-note
imports fail before any write.

## Parsing and limits

Format 0/1 with PPQ timing; format 2 and SMPTE are rejected. Supports running status,
velocity-zero note-off, FIFO matching of overlapping same-pitch attacks, conductor tempo
and track names. Missing note-offs close at their track's end with a warning. SysEx,
controllers (including sustain), pitch bend and programs are ignored with warnings.
Tempo maps flatten to starting tempo, bounded to 30–240 BPM; hardware tempo stays intact.
The app's bar grouping is 4/4; MIDI time signatures do not change note timing.

Bounded to 4 MB, 256 file tracks, 250,000 events, 20,000 notes and 4096 four-quarter bars.
Chunk lengths, event data and four-byte variable integers are checked before reading.
Workspace/source changes invalidate pending import reviews. Device operation cancellation
follows the connection epoch; a reconnect invalidates the hardware review. Transfer waits
for preview playback to finish releasing notes. Failed hardware transfer can be partial;
reconnect and Read.

## Validation

`Sloop.MidiFile.Tests` covers format 1 conductor tracks, channel splitting, running status,
overlapping pitches, all file-prefix truncations, invalid data/chunks/timing, unsupported
formats, warnings, PPQ conversion, undo, every DIV, four-voice limits, ties and GM audition.
It also runs 1,000 deterministic mutated-file probes. Workstation integration tests
exercise native wire application, readback, unaffected tracks and stale-baseline refusal.
Phone picker/layout and physical FM1 playback remain pending live validation.
