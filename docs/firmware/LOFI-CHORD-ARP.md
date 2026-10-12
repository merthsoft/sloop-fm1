# LOFI chord arpeggios

LOFI → MOTN → ARP keeps the original OFF, OCT, MAJ and MIN values unchanged so
existing songs and presets retain their sound. Additional choices cover TRIAD,
7TH, 9TH, SUS4, POWER, SUS2, ADD9, 6TH, SHELL, OCTAVE, DOM7,
MAJ7, MIN7, DIM, AUG, HALFDIM and DIM7. The duplicate MAJOR/MINOR entries
are omitted: the original MAJ/MIN modes already provide those fixed triads.

New choices use the same chord builder as SEL: degree shapes follow the track's
ROOT/SCALE (with the existing CHR/minor fallback); fixed qualities retain their
semitone intervals. CHROM uses the literal played root. Each chip voice cycles its
valid chord pitches at the existing approximately 50 Hz cadence. MIDI range-end
notes are omitted safely; it does not allocate additional chord voices or enable
the separate track CHORD mode. Using both is possible but produces layered arps.

This is LOFI's rapid internal chip arpeggio, not the tempo-synced track ARP.
No new project format or persistent RAM is introduced. Tests preserve all legacy
offsets, compare every SEL shape/scale/root across MIDI 0–127, and independently
check four-note major-seventh and half-diminished cycles.
