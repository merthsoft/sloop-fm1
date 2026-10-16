# Custom native progressions

Add CUSTOM after the 24 ROM starters. It uses the same root, scale, octave,
CHORD/BASS/ARP NOTES, VLEAD, rhythm, shaping, preview, ADD/REPLACE and undo path.
Keep the existing catalog IDs stable for phone clients.

A fourth SELECT page, CUSTOM, provides COUNT (1–8), SLOT (1-based), DEGREE
(1–7, interpreted in the selected scale), and SPAN (4/8/16 sixteenth-note steps
per chord). Editing any progression control selects CUSTOM. Increasing COUNT
reduces SPAN when necessary to fit the 64-step track; increasing SPAN stops at
capacity. New positions retain their last values; the initial recipe is 1–5–6–4,
with the remaining slots initialized to tonic. Show all active degrees (the count is visible as the number of positions), with a
selected-position marker and span readout. Three chords at span 16 take
48 steps; six at span 8 take 48. Preview wraps at that actual length, as does REPLACE.
ADD retains the destination track length and existing capacity safeguards.

One shared draft lasts until power-off; applying it writes ordinary editable
song steps, saved with the song. No new project format or flash persistence.
Page/slot browsing preserves preview; changing the musical recipe stops it.
Unchanged ROM starter output and phone catalog behavior are regression contracts.
Test count/span limits, exact degrees, lengths and rhythms, preview/apply parity,
voice-leading, switching tracks, ADD and complete undo. Measure target flash/RAM.

## Implemented and verified

The native page and shared renderer are implemented. All 24 ROM starters remain
unchanged in a 100,000-event comparison against the prior renderer (frozen hash
627dceea for the three-scale test harness). Tests cover counts 1–8, bounded spans,
all rhythms/modes, shaped short phrases, actual knob input, preview wrap and
Apply parity, track isolation, undo, capacity guards and browser navigation.
The custom draft adds no project fields or pattern buffers. The renderer shares
plain and voice-led logic; ROM degrees use 3 bits each plus a seventh flag.
The build measures 581,560 flash bytes (4 free), static RAM 82,116 bytes and
pool 332,876 bytes. Hardware audition remains user testing.
