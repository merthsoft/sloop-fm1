<p align="center"><img src="assets/logo/sloop-logo.png" alt="SLOOP" width="420"></p>

# SLOOP 2.7 — The Complete Guide

Everything SLOOP does, every button, every combination, every page, in one place. This guide is written to be read from the top the first time, then used as a reference: the [cheat sheet](#26-cheat-sheet) at the end has every combination on one page.

SLOOP is a free, open-source (GPL-3.0) firmware for the M-VAVE FM-1, based on [Felucca](https://github.com/hugelton/Felucca) by Leo Kuroshita / Hügelton Instruments. The short manual is [SLOOP.md](SLOOP.md); the project page is [README.md](README.md).

---

## How to read this guide

| Written | Means |
| --- | --- |
| **tap FX** | press FX and let go without touching anything else (shorter than about half a second) |
| **hold FX** | keep FX pressed |
| **FX + key 3** | hold FX, and while it is held press white key 3 |
| **FX + KNOB 2** | hold FX, and while it is held turn KNOB 2 |
| **SEQ + step + KNOB 4** | hold SEQ, hold a step key, turn KNOB 4 |
| **key 1 … key 16** | the 16 white keys, from the lowest (**F3** = key 1) to the highest (**G5** = key 16) |
| **GO** | a value that does something (LOAD, SAVE, NEW…): turn it one click, *AGAIN: …* shows, turn it one more click within about 1.5 s to do it |

The 16 white keys, numbered:

| Key | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 | 10 | 11 | 12 | 13 | 14 | 15 | 16 |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| Note | F3 | G3 | A3 | B3 | C4 | D4 | E4 | F4 | G4 | A4 | B4 | C5 | D5 | E5 | F5 | G5 |

The 11 black keys are F#3, G#3, A#3, C#4, D#4, F#4, G#4, A#4, C#5, D#5, F#5. The "first four black keys" are F#3, G#3, A#3 and C#4.

---

## Contents

1. [Install and update](#1-install-and-update)
2. [The panel](#2-the-panel)
3. [The big ideas: tracks, colours, tap and hold](#3-the-big-ideas-tracks-colours-tap-and-hold)
4. [Your first beat in 60 seconds](#4-your-first-beat-in-60-seconds)
5. [Every button, alone](#5-every-button-alone)
6. [Playing the keys](#6-playing-the-keys)
7. [The drum track](#7-the-drum-track)
8. [Recording](#8-recording)
9. [The layers, one by one](#9-the-layers-one-by-one)
10. [The step sequencer: steps, nudge, locks, fills](#10-the-step-sequencer-steps-nudge-locks-fills)
11. [Key, scales, chords and CHORD+](#11-key-scales-chords-and-chord)
12. [Mix: levels, mute, solo, fills, tap tempo](#12-mix-levels-mute-solo-fills-tap-tempo)
13. [Undo, clear, save, projects, autosave](#13-undo-clear-save-projects-autosave)
14. [Song mode: sections, chains, songs](#14-song-mode-sections-chains-songs)
15. [The screens](#15-the-screens)
16. [Every page, knob by knob](#16-every-page-knob-by-knob)
17. [The sounds and the twelve engines](#17-the-sounds-and-the-twelve-engines)
18. [Effects: master, punch-in, per track](#18-effects-master-punch-in-per-track)
19. [The visualiser](#19-the-visualiser)
20. [The HOME menu and the lights](#20-the-home-menu-and-the-lights)
21. [MIDI](#21-midi)
22. [USB audio](#22-usb-audio)
23. [Your own samples and drum kits](#23-your-own-samples-and-drum-kits)
24. [The web editor](#24-the-web-editor)
25. [Backup, rescue, going back, troubleshooting](#25-backup-rescue-going-back-troubleshooting)
26. [Cheat sheet](#26-cheat-sheet)

---

## 1. Install and update

**From the browser (recommended).** Open the [installer page](https://isod89.github.io/sloop-fm1/) in **Chrome or Edge**, connect the FM-1 by USB (a data cable, plugged straight in, no hub), press **INSTALL**, allow MIDI access, and wait for *Done*. The FM-1 restarts on the SLOOP logo.

**From the source folder (Windows).** Double-click **`INSTALL-SLOOP.bat`**: it builds the firmware and opens the installer at `http://localhost:8766/webapp/installer/`. Keep the black window open until the install is done. **`OPEN-EDITOR.bat`** opens the web editor.

**Updating** keeps your projects, presets, samples, kits and settings. The installer checks the package (SHA-256), SLOOP's update loader checks it again (CRC) before it starts the new firmware, and a damaged package is refused before anything is written.

**Before you go back to an older SLOOP** (2.3 or earlier), save a backup in the editor: 2.3 cannot read 2.4 projects (format FUN5).

---

## 2. The panel

| Control | Kind | Main job |
| --- | --- | --- |
| **PLAY** | button | start / stop all four tracks |
| **REC** | button | record (tap), clear the track (hold) |
| **SAVE** | button | song layer (hold), song screen / save pages (tap) |
| **HOME** | button | TRACKS screen, visualiser (tap), menu (hold), lock a layer |
| **ENV** | button | envelope pages |
| **LFO** | button | LFO pages |
| **FX** | button | punch-in layer (hold), effect pages (tap) |
| **SEL** | button | key / chord layer (hold), scale pages (tap). The second button of the top row, between FX and ENV (not the SELECT knob) |
| **EDIT** | button | erase layer (hold), engine pages (tap) |
| **ARP** | button | note-repeat layer (hold), arpeggiator pages (tap) |
| **SEQ** | button | step layer (hold), sequencer pages (tap) |
| **GLO** | button | mix layer (hold), global pages (tap) |
| **OCT− / OCT+** | buttons | octave, ghost / hard hits, undo / redo, pages |
| **ALGORITHM** | knob | selects the track (1–4) |
| **PRESETS** | knob | the selected track's sound, or the drum kit |
| **SELECT** | knob | tempo, or the previous / next page |
| **KNOB 1–4** | knobs | what the four dials at the bottom of the screen show |
| **MASTER** | knob | output volume (headphones / line out) |
| **27 keys** | keys | 16 white (F3–G5) and 11 black |

---

## 3. The big ideas: tracks, colours, tap and hold

### Four tracks

| Track | Colour | Plays | Knob of the same colour |
| --- | --- | --- | --- |
| **1** | blue | a synth (3 synth tracks share 8 voices) | KNOB 1 |
| **2** | green | a synth | KNOB 2 |
| **3** | yellow | a synth | KNOB 3 |
| **4** | orange | the drum machine: 16 sounds, one per white key | KNOB 4 |

**ALGORITHM** picks the track you play, record and edit, on every screen. The screen and the whole editor take the colour of the selected track. **White** on the screen always means *what you are touching*; **red** always means *recording*.

### Tap and hold

Every function button has two lives:

- **Tap** it: its **pages** open (sound design, settings), as on any FM-1 firmware. Tap it again: the next page of that button. **SELECT** also walks the pages of that button, both ways. Each button remembers the last page you used.
- **Hold** it: a **layer**. While it is held, the 16 white keys and KNOB 1–4 do something else, and after 0.14 s the screen shows the 16 keys as 16 tiles (four rows of four: keys 1–4, 5–8, 9–12, 13–16) and the knobs as dials. Let go: back to playing.

The layers are **FX** (punch-in effects), **EDIT** (erase), **ARP** (note repeat), **SEQ** (steps), **SEL** (key and chords), **GLO** (mix) and **SAVE** (song). ENV and LFO have no layer: they only open pages.

**Landmarks:** while a layer is held (and always on the drum track) keys 1, 5, 9 and 13 glow dimly — the first key of each row of tiles — so you can find a tile without looking. Keys at full light are what is on.

**Lock a layer:** hold its button and **tap HOME**. The layer stays open with the button let go (*LOCK* on the screen, the button blinks): both hands free, one on the keys, one on the knobs. Any other button lets it go (HOME, the layer's own button, ENV…) and does only that; **PLAY, REC and OCT− / OCT+ keep working** inside a locked layer.

**Inside any layer:** PLAY still starts and stops, **SELECT** is always the tempo, and the ALGORITHM / PRESETS knobs wait (no jump when you let go).

---

## 4. Your first beat in 60 seconds

1. Turn **ALGORITHM** to track **4** (orange). The white keys play 16 drum sounds: **F3 kick**, G3 kick 2, A3 snare, B3 clap, **C4 hat**, D4 open hat… Turn **PRESETS** to pick a kit: try *808* or *BOOMBAP*.
2. Press **REC** (*rec ready*). **Play a beat freely**, at your own tempo — no click, no count-in. Hold **OCT−** while you hit for ghost notes, **OCT+** for hard hits.
3. **Press REC on the "1" after your last bar.** The loop closes: its length sets the tempo, the hits snap to the grid, and it plays at once.
4. Press **REC** again while it plays: you record on top (overdub). Hold **ARP** and hold the hat key (C4): a 1/16 hat roll, recorded as ratchets.
5. Turn **ALGORITHM** to track **1** (blue, *808 BOOM*), press **REC**, play a bass line.
6. Hold **SEL** and press the key of your song (for example D). Select track 2, hold **SEL** and turn **KNOB 1** to *7TH*: every white key is now a chord of the key, one finger each.
7. Hold **FX** and hold a key (white or black) for a punch-in effect; still holding FX, turn **KNOB 2** for DUST, **KNOB 3** for DUCK.
8. A mistake? **EDIT + OCT−**: undo.

---

## 5. Every button, alone

| Control | Tap / turn | Hold |
| --- | --- | --- |
| **PLAY** | start / stop all tracks (in song mode: play the song). Works inside every layer | — |
| **REC** | playing: record now / stop recording · stopped: arm (*rec ready*) · free take: close the loop · count-in: cancel | ~2 s: **clear the selected track** (a ring fills; let go before it is full and nothing happens) |
| **SAVE** | on TRACKS or DRUMS: the **SONG** screen · on the SONG screen: save the song · elsewhere: the **SAVE pages** | the **song layer** (sections A–D, chain, song mode, SONG REC) |
| **HOME** | the **TRACKS** screen · on TRACKS: the **visualiser** (HOME again closes it) | 0.7 s: the **menu** (SCREEN, LIGHTS, AUDIO, SYSTEM); hold again to leave it |
| **ENV** | ENV pages (ENV, ENV DEST) | — |
| **LFO** | LFO pages (LFO, LFO DEST) | — |
| **FX** | FX pages (FX, FILTER, SLICER, DLY, REV/CHO) | punch-in layer |
| **SEL** | SEL pages (SEL, SEL 2) | key / chord layer |
| **EDIT** | EDIT pages (EDIT 1, EDIT 2, VOICE, VOICE 2) · on TRACKS with the drum track: the **DRUMS** screen · on DRUMS: grid → kit → groove · on the STEP page: clear the step | erase layer |
| **ARP** | ARP pages (ARP, ARP 2) | note-repeat layer |
| **SEQ** | SEQ pages (STEP, NOTES, NOTE ACTIONS, PATTERN, RECORD, SEQUENCES, SONG) · on TRACKS with the drum track: the **DRUMS** screen · on DRUMS: grid → kit → sound → groove → rhythm | step layer |
| **GLO** | GLO pages (GLOBAL, MASTER, SYSTEM, DRUMS) | mix layer |
| **OCT− / OCT+** | synth track: octave down / up (−3 … +3) · both together: octave 0 | drum track: **ghost** (OCT−) / **hard** (OCT+) hits while held |
| **ALGORITHM** | select track 1–4, on every screen (inside a layer it waits; not during a free take, on the SONG screen or in the menu) | — |
| **PRESETS** | on TRACKS, HOME and the PRESETS page: the selected track's **sound** (all factory sounds by kind, then your user presets) · drum track: the **kit** · on the DRUMS screen: the kit | — |
| **SELECT** | on TRACKS, inside a layer and on the REC screen: the **tempo** · on a page: previous / next page of its button (stops at the ends) · DRUMS screen: grid → kit → groove · visualiser: the style · menu: the section | — |
| **KNOB 1–4** | the four dials at the bottom of the screen | — |
| **MASTER** | output volume (the visualiser ignores it) | — |

Startup shows the firmware title; the bitmap splash is omitted to keep room for the combined sound and performance features. Your saved work still loads normally.

### Combinations at power-on and while stopped

| Combination | Does |
| --- | --- |
| **OCT−** held while switching on | **USB rescue** (*SLOOP USB RESCUE*): install again from the browser |
| **OCT− + OCT+** held while switching on | **hardware calibration**: teach each button and knob (also in the HOME menu → SYSTEM) |
| **OCT− + OCT+** held 5 s, stopped | **update mode** (UBOOT): a countdown shows from 2 s; let go to cancel. Your work is saved first |

---

For the additions specific to this branch, see the [illustrated MerthSloop firmware guide](docs/firmware/MERTHSOFT-FEATURES.md).

## 6. Playing the keys

### Synth tracks (1–3)

- The keys play the selected track. The notes you play always go out on USB MIDI (the track's channel).
- **OCT− / OCT+**: octave −3 … +3; both together: back to 0. The OCT lights show when you are off 0.
- What the keys play depends on the track's **KEYS** setting and **CHORD** mode (SEL layer or SEL page):

| KEYS | CHORD | The keys play |
| --- | --- | --- |
| **OFF** | OFF | every key chromatic (the default) |
| **SNAP** | OFF | every key, rounded down to the scale |
| **WHITE** | OFF | the white keys walk the scale from C4 (C4 = the key's root); black keys are silent |
| **OFF / SNAP / WHITE** | **on** | the white keys play **chords** of the scale, one per key (C4 = the I chord); the black keys **change the chord** ([CHORD+](#chord-the-black-keys-change-the-chord)) |
| **CHROM** | **on** | every piano key plays a chord rooted on its actual note; black keys are roots rather than modifiers. OCT and TRN still apply; ROOT does not shift these literal keys. |
| **CHROM** | OFF | every key chromatic, like OFF |

On the SCL page, set **QNT / KEYS = CHROM** for chromatic chord roots. For example, choose
**CHORD = MAJOR** and press C# to play C# major. Fixed chord shapes retain their quality;
scale-derived shapes use the selected scale's tonic quality transposed to each root.
**SCALE = CHR** means all twelve semitones for ordinary notes, but legacy scale-derived
chords deliberately use minor harmony when CHR is selected. Choose an explicit MAJOR,
MINOR, DOM7, etc. for an unambiguous quality. Existing projects keep their key mapping.

- The **GM KIT** sound (SAMPLE engine) is the exception: the lowest key is a kick (C2), no scale.

### Drum track (4)

16 sounds, one per white key; a black key plays the sound of the white key on its left (two fingers on one sound, for fast rolls). Hold **OCT−** for ghost notes, **OCT+** for hard hits. See [The drum track](#7-the-drum-track).

### Velocity and levels

The FM-1's keys have no velocity: on the drum track every hit has one of four **levels** — **GHOST**, **SOFT**, **NORM** (as played), **HARD** — chosen with OCT− / OCT+ as you play, or later in the step sequencer. On synth tracks a step can carry an **accent** (STEP page, FLAG).

---

## 7. The drum track

### The 16 sounds

| Key | Sound | Key | Sound | Key | Sound | Key | Sound |
| --- | --- | --- | --- | --- | --- | --- | --- |
| 1 · F3 | KICK | 5 · C4 | HAT | 9 · G4 | SNARE 2 | 13 · D5 | RIDE |
| 2 · G3 | KICK 2 | 6 · D4 | OPEN HAT | 10 · A4 | LOW TOM | 14 · E5 | SHAKER |
| 3 · A3 | SNARE | 7 · E4 | PEDAL HAT | 11 · B4 | HI TOM | 15 · F5 | CONGA |
| 4 · B3 | CLAP | 8 · F4 | RIM | 12 · C5 | CRASH | 16 · G5 | COWBELL |

- **Levels:** GHOST, SOFT, NORM, HARD (hold OCT− / OCT+ while you hit). Softer hits are darker as well as quieter.
- **Ratchets:** a hit can repeat x1–x4 inside its step (ARP rolls record them; SEQ + step + KNOB 3 sets them).
- **Choke:** the closed hat and the pedal hat cut the open hat.
- **6 drum voices**, separate from the synths' 8.

### Kits

**PRESETS** on the drum track (or KNOB 1 on the DRUMS kit page, or the editor) picks the kit:

| # | Kits |
| --- | --- |
| 1–5 | **ACOUSTIC** (sampled studio kit, CC0) and its treatments: DEEP, TIGHT, BRIGHT, DUST |
| 6–37 | 32 synthesised kits, 16 sounds each: 808, 909, 606, 80S, VINTAGE, TRAP, DRILL, BOOMBAP, LO-FI, PHONK, HOUSE, D.HOUSE, TECHNO, MINIMAL, ELECTRO, DISCO, GARAGE, JUNGLE, DUBSTEP, DEMBOW, AMAPIANO, AFRO, LATIN, TRIBAL, SYNTHWV, CHIP, ARCADE, GLITCH, INDUSTR, HYPER, AMBIENT, JAZZ |
| 38–41 | **USR1, USR2, USR3, USR4**: your own kit in one sample slot (about 7.4 s) |
| 1 | **USR3+4**: your own big kit over two slots (about 15 s) |
| 43–46 | **SYN1, SYN2, SYN3, SYN4** (2.5): your own synthesised kits, made in the editor's [Drum synth](#your-own-synthesised-kits-syn1syn4) page. The FM-1 shows the name you gave them |

Every kit is level-matched. The kit is saved with projects and song sections. Changing the kit never touches the pattern. [Your own kits](#23-your-own-samples-and-drum-kits) work like any other: steps, levels, ratchets, fills, choke, MIDI.

### The DRUMS screen

Open it with **EDIT** or **SEQ** tapped on the TRACKS screen while the drum track is selected. It has five pages: **GRID**, **KIT**, **SOUND**, **GROOVE**, **RHYTHM**. SOUND selects a drum lane and edits its tune/decay; GROOVE selects the built-in beat; RHYTHM shapes it. **EDIT / SEQ tapped** cycles them; **SELECT** moves between them.

**Grid page** — the 16 sounds × 16 steps, levels as shades, ratchets as notches.

| Control | Does |
| --- | --- |
| KNOB 1 **sound** | picks the sound (lane) — you hear it |
| KNOB 2 **step** | moves to a step — you hear what it holds |
| KNOB 3 **hit** | right: a hit of that sound on that step (NORM) · left: clear it |
| KNOB 4 **level** | the hit's level up / down (GHOST, SOFT, NORM, HARD) |
| **white keys** | the 16 steps of the sound KNOB 1 picks: press to set the step (you hear it), again to clear it; the keys light that sound's steps |
| **first four black keys** | the page of steps: 1–16, 17–32, 33–48, 49–64 |
| **PRESETS** | the kit |
| **SAVE** | the SONG screen |
| **HOME** | back to TRACKS |

**Kit page** — 16 pads that flash on every hit; the keys play the pads.

| KNOB 1 | KNOB 2 | KNOB 3 | KNOB 4 |
| --- | --- | --- | --- |
| **kit** | **level** of the drum track | **reverb** send of the drums | **pan** of the drum track |

<a id="drum-sound"></a>**Sound page (2.7)** — each of the 16 sounds tuned and shortened or lengthened, on every kit. Play a key (or turn KNOB 1) to pick the sound: its pad is framed, and a sound you changed shows a small square.

| KNOB 1 | KNOB 2 | KNOB 3 | KNOB 4 |
| --- | --- | --- | --- |
| **sound** (you hear it) | **tune**: −8 … +7 semitones | **decay**: −4 (very short) … 0 (as the kit has it) … +3 (long) | **level** of the drum track |

On the synthesised kits (808, 909… and your SYN1–4) DECAY shortens or lengthens the sound itself; on the sampled kits (ACOUSTIC … DUST, USR) it can only shorten it (a fade), +3 plays the sample as it is. The settings are kept with the project and with each section A–D, so a section can have its own tuning; changing the kit keeps them, NEW sets them back to 0. A project from an older SLOOP opens with every sound as its kit has it.

While the song plays in song mode, the grid's hit and level knobs wait (*STOP THE SONG FIRST*).

**Groove page** — build a beat directly on the FM-1. Choose from 24 starters, including house, techno, disco, hip-hop, drum-and-bass, garage, reggaeton, bossa and AMEN BREAK / AMEN HALF. The screen previews kick, snare, clap and hats.

| KNOB 1 | KNOB 2 | KNOB 3 | KNOB 4 |
| --- | --- | --- | --- |
| **groove** starter | preview **step** | preview **sound** | **audition** that hit using the selected kit |

Press **OCT+** to **apply**; if the drum pattern contains anything, press **OCT+ again** or hold it continuously for 700 ms to confirm replacement. **OCT− toggles a looping preview** through the current kit, without replacing your beat. During replacement confirmation, OCT− cancels and returns to GRID. Browsing leaves your pattern intact. Stop playback and recording first; an armed recording also blocks apply (*STOP FIRST*).

Apply replaces the drum steps, clears their ratchets, nudges, locks and fill conditions, and sets the starter's length and division. Most straight starters are 16 sixteenth-note steps; AMEN BREAK is 64 steps (four bars) and AMEN HALF is 32 steps (two bars). The preview grid follows the selected step in 16-step windows; SHUFFLE is 12 eighth-note triplet steps. Local drum swing becomes zero; global swing remains selected. The kit, mixer, tempo, song key and synth tracks stay selected. SMALL FILL repeats as an ordinary pattern. Edit any starter with GRID, SEQ or live recording, and save it as usual. **EDIT held + OCT− / OCT+** undoes/redoes replacement, including the overwritten timing and locks, while stopped.

---

## 8. Recording

SLOOP records live and layers every pass on top of the last (overdub). Notes go to the nearest step **as you heard it**: the ~12 ms between a key and its sound are taken back, so what you play on the beat lands on the beat. Chords are kept on the synth tracks whose VOICE is POLY (up to 4 notes a step; a MONO, LEGATO or UNISON sound, as most basses and leads, plays and records one note at a time); held notes become ties. The TRACKS page draws one mark a step; the STEP page shows a chord's other notes (*+1*, *+2*…).

**Over held notes (2.7).** On a POLY track, a note you record while held notes of the pattern ring (a pad tied over the bar) no longer cuts them: they go on under it, and your note ends where you let it go. Up to 4 notes sound at once on a track (a fifth makes the oldest held one end there). Entering steps by hand (SEQ + key) still replaces what plays, so a long note can be cut on purpose. On SLOOP 2.6 such a pattern plays the held notes again where a note was laid over them.

**With a MIDI keyboard:** REC records the **selected** track. A keyboard on channel 1, 2 or 3 always plays track 1, 2 or 3 (2.6: the channels of GLO → MIDI), whatever track is selected: set it to channel 4 (or 4–16) to play and record the selected track. From 2.6, if your keyboard plays another track than the one recording, the screen says *MIDI PLAYS TRACK n* (once a take).

### What REC does

| When | REC does | Then |
| --- | --- | --- |
| **Playing** | records the selected track **at once** | REC again stops recording; the loop plays on |
| **Stopped, project with notes** | arms (*rec ready*, the REC light blinks) | **your first note starts the loop and is step 1**; PLAY starts it too |
| **Stopped, empty project** | arms (*rec ready · play freely*) | a **free take**, or, with MODE *tempo*, as with notes |

REC again while armed disarms it (*REC OFF*). In **song mode**, REC waits until the song is stopped (*STOP THE SONG FIRST*), and arming REC switches back to loop mode.

While recording, the REC light is solid and the track shows a red *rec*. **Turn ALGORITHM** while recording and the take moves to the next track without stopping.

### The REC screen (armed, before the first note)

| Knob | Dial | Choices |
| --- | --- | --- |
| KNOB 1 | **mode** (empty project only) | **free**: a free take, the tempo follows you · **tempo**: record at the tempo set (SELECT) |
| KNOB 2 | **length** | the selected track's loop: **1, 2 or 4 bars** |
| KNOB 3 | **start** | **note**: your first note starts the loop · **count**: press **PLAY**, one bar of clicks (4, 3, 2, 1 on the screen), then recording starts; notes before only sound |
| KNOB 4 | **quant** (2.7) | **on**: what you record lands on the steps (as before) · **off**: it keeps your timing (see below) |

LENGTH and START apply when you record at a tempo (MODE *tempo*, or a project that already has notes); a free take makes its own length. MODE, START and QUANT are settings of the FM-1 (they stay as you left them, also for recording while the song plays).

<a id="quant-off"></a>**QUANT off (2.7).** A note you play still goes into the nearest step, and how far off the grid you played it becomes that step's **nudge** (SEQ + step + KNOB 4: 1/64 of a step, about 2 ms at 120 BPM in 1/16). So the pattern plays back with your groove, and you can still edit it step by step (a nudged step shows its dot; SEQ + step + OCT− puts it back on the grid). It works for the keys and MIDI, in the free take too; the arp and note repeat stay on their grid. A step has one nudge: the first note in a step sets it, and the other notes of a chord or other drum sounds on that step follow it (in DIV 1/32 the steps are twice as fine). Undo (EDIT + OCT−) takes a take's nudges back with its notes. During the count-in, **REC cancels it** and **PLAY** goes back to *rec ready*. SELECT sets the tempo on this screen.

### Free take: the loop follows you

1. On an empty project, **REC**, then play freely. The screen shows *free take*, the seconds, and the loop it would make right now (*2 bars · 92 bpm*).
2. **Press REC on the "1" after your last bar.** SLOOP picks 1, 2 or 4 bars at the tempo nearest the one set (within 3 % the set tempo is kept), writes your notes with their lengths and levels, and plays the loop at once. All four tracks take that length.
3. **PLAY** during a free take drops it. A take closes by itself after 24 s.

### Metronome and swing

- The **PLAY light flashes on every beat**: a silent metronome.
- An audible click: **GLO → GLOBAL → CLICK** = **OFF**, **REC** (only while a track records) or **ON** (whenever it plays). It is a short tick of its own (higher on the first beat of a bar), heard with every kit, even with the drum track muted or another track soloed; its level follows GLO → DRUMS → LVL. The click is never recorded, and the count-in always clicks.
- **Swing** goes from **0** (straight) to **100** (the strongest: a step pair played 75 / 25, as MPC swing at 75 %): **HOME, KNOB 1** or **GLO → GLOBAL → SWING** for all tracks, **SEQ + KNOB 3** (or SEQ → PATTERN → SWG) per track; a track's swing adds to the global one. Triplet divisions are never swung.

### What gets recorded

- **Synth tracks:** the notes (and chords) with their lengths; ties for held notes; ARP rolls as ratchets; CHORD+ chords as they sound.
- **Drum track:** the hits with their levels; ARP rolls as **ratchets** (a 1/32 roll on a 1/16 track: x2 on each step).
- Never recorded: punch-in effects, the click, notes played in a layer that uses the keys for something else (FX, SEQ, GLO…).

---

## 9. The layers, one by one

All nine layers at a glance:

| Hold | Name | White keys | KNOB 1 | KNOB 2 | KNOB 3 | KNOB 4 | Tap |
| --- | --- | --- | --- | --- | --- | --- | --- |
| **LFO** | vibrato | playable keyboard | Hz | depth | wave | beat sync | normal LFO pages |
| **ENV** | tremolo | playable keyboard | Hz | depth | wave | beat sync | normal ENV pages |
| **FX** | punch | a punch-in effect while the key is held | FILTER (master) | DUST | DUCK | TRK FILT (selected track) | FX pages |
| **EDIT** | erase | erase that sound / note from the pattern | SHIFT | LENGTH ×2 / ½ | TRANSPOSE | — | EDIT pages |
| **ARP** | roll | note repeat on the grid | RATE | — | — | — | ARP pages |
| **SEQ** | steps | steps 1–16 of the page | SOUND / NOTE | DIV | SWING | LENGTH | SEQ pages |
| **SEL** | key | the key of the song | CHORD | SCALE | KEYS | TRANSPOSE | SEL pages |
| **GLO** | mix | 1–4 mute · 5–8 solo · 9 fill · 10 fill bar · 16 tap tempo | level 1 | level 2 | level 3 | level 4 | GLO pages |
| **SAVE** | song | 1–4 play A–D · 5–8 save A–D · 13 loop / song · 14 SONG REC · 16 song screen | — | — | — | — | song screen / SAVE pages |

### FX — punch

| Control | Does |
| --- | --- |
| **FX + a key** | the [punch-in effect](#punch-in-effects) of that key (16 white keys, and 11 black keys, 2.6), on the whole mix, for as long as the key is held |
| **FX + KNOB 1** | master **FILTER**: left a low-pass, right a high-pass, centre off |
| **FX + KNOB 2** | master **DUST** (old sampler and vinyl) |
| **FX + KNOB 3** | master **DUCK** (sidechain pump from the kick) |
| **FX + KNOB 4** | **TRK FILT**: the same one-knob filter on the **selected track only** (any track, the drums too) |
| **FX + OCT−** (held, 2.6) | **REV THROW**: every track's reverb send (the drums' too) goes full while held, gliding in and out (~12 ms); with a punch-in key too. The song's REV values never change, nothing is saved or recorded |
| **FX + OCT+** (held, 2.6) | **ECHO THROW**: the same for the delay sends, and FDBK at least 70 % while held. Both together: both throws |
| **FX + HOME** | lock the FX layer open (both hands free) |

Keys pressed while FX is held never play or record notes.

### EDIT — erase

| Control | Does |
| --- | --- |
| **EDIT + key**, playing | erases that sound (drums) or note (synths; with CHORD on, its chord) from **every step the playhead passes while you hold the key** — MPC style: hold the hat key for one bar and that bar's hats are gone. *ERASED* flashes |
| **EDIT + key**, stopped | erases that sound / note from the **whole pattern** at once |
| **EDIT + KNOB 1** | **SHIFT**: every step one later (right) / earlier (left) — turns the groove around |
| **EDIT + KNOB 2** | **LENGTH**: right ×2 (the pattern copied after itself, up to 64 steps), left ½ |
| **EDIT + KNOB 3** | **TRANSPOSE**: every note a semitone up / down (synth tracks) |
| **EDIT + KNOB 4** | **OCTAVE**: every note a whole octave up / down (synth tracks); stops at MIDI bounds without collapsing chord intervals |
| **EDIT + OCT−** | **undo** |
| **EDIT + OCT+** | **redo** |
| **EDIT + a black key** (drum track, 2.6) | **MUTE mode**: the white keys now **mute / unmute their sound** in the sequencer, live, without erasing anything (the muted sounds show *OFF* in red; the screen says *mute: key = sound*). A black key again goes back to erasing. 2.7: MUTE mode **stays** when you let go of EDIT, so the next EDIT + key mutes at once (2.6 went back to erasing) |

All the knob turns of one EDIT hold count as one change for undo.
The dials show the last gesture's SHIFT in steps, total TRANSPOSE in semitones,
and the octave-knob amount. Undo shows zero; redo restores the amounts. A new
gesture starts fresh. Synth EDIT tiles show a scale guide with physical-key hints;
they do not remap which notes the keys erase.

The drum mutes stay on until you unmute the sound (EDIT + a black key, then its key again); they are a performance tool, not part of the pattern: a project loaded or a new one starts with every sound playing. A muted sound still plays from the keys and from MIDI, and its ratchets are muted too.

### ARP — roll (note repeat)

| Control | Does |
| --- | --- |
| **ARP + key (held)** | the key repeats on the grid at the RATE, locked to the tempo and the swing; it ends with the key |
| **ARP + KNOB 1** | **RATE**: 1/8, 1/16, 1/32, 32T, 1/64 (also GLO → MASTER → ROLL) |
| **ARP + key + OCT− / OCT+** | on the drum track: ghost / hard rolls |

While recording, a roll is written as ratchets (a 1/32 roll on a 1/16 track: x2 on each step). Rolls ignore fills. (The **arpeggiator** is something else: tap ARP for its pages.)

### SEQ — steps

The 16 white keys are the 16 steps of the current page; the lit ones play.

| Control | Does |
| --- | --- |
| **SEQ + empty step** | sets it at once — drums: with the sound shown (KNOB 1, or the last pad you hit); synths: with the note or chord you played last (on the keys, or a MIDI keyboard: 2.6) |
| **SEQ + set step (tap)** | clears it (with its nudge, locks and fill condition) |
| **SEQ + several step keys held** | edit them together |
| **SEQ + first four black keys** or **SEQ + OCT− / OCT+** (no step held) | the page: steps 1–16, 17–32, 33–48, 49–64 (up to the track's length) |
| **SEQ + KNOB 1** (no step held) | the **sound** (drums, you hear it) / **note** to set |
| **SEQ + KNOB 2** | **DIV**: 1/4, 1/8, 1/16, 1/32, 8T, 16T, 1/2, 1BAR, 2BAR |
| **SEQ + KNOB 3** | **SWING** of the track |
| **SEQ + KNOB 4** | **LENGTH**: 1–64 steps |
| **SEQ + step + KNOB 1** | drums: which **sound** of the step LEVEL and RATCHET edit (you hear it) · synths: the step's **note** (every note of a chord moves together) |
| **SEQ + step + KNOB 2** | drums: **LEVEL**: ghost, soft, norm, hard · synths (2.6): **VEL**, the step's velocity 1–127 (every note of the step; an accent is taken off, as it would play at 127) |
| **SEQ + step + KNOB 3** | **RATCHET**: x1–x4 |
| **SEQ + step + KNOB 4** | **NUDGE**: −32 … +31 (in 1/64 of a step) |
| **SEQ + step + PRESETS** | a **parameter lock** on that step: the first click makes it one click away from the track's value, the next ones move it |
| **SEQ + step + ALGORITHM** | which parameter the lock is on |
| **SEQ + step + OCT+** | the step's **fill condition**: normal → FILL ONLY → NO FILL → normal |
| **SEQ + step + OCT−** | clears the step's nudge, locks and fill condition |

A held step that you edited with a knob is **kept** when you let go (only a plain tap clears). Details in [The step sequencer](#10-the-step-sequencer-steps-nudge-locks-fills).

### SEL — key and chords

| Control | Does |
| --- | --- |
| **SEL + any key** | sets the **key of the song**: the root of all three synth tracks (*KEY D*) |
| **SEL + KNOB 1** | **CHORD** of the selected synth track: OFF, TRIAD, 7TH, 9TH, SUS4, POWER, SUS2, ADD9, 6TH, SHELL, OCTAVE, MAJOR, MINOR, DOM7, MAJ7, MIN7, DIM, AUG, HALFDIM, DIM7 |
| **SEL + KNOB 2** | **SCALE** of all three synth tracks: 16 scales |
| **SEL + KNOB 3** | **KEYS**: OFF, SNAP, WHITE |
| **SEL + KNOB 4** | **TRANSPOSE** the selected track, ±24 semitones |

Details in [Key, scales, chords and CHORD+](#11-key-scales-chords-and-chord).

### GLO — mix

| Control | Does |
| --- | --- |
| **GLO + key 1–4** | **mute** / unmute track 1–4 (fades out in a few ms; the pattern runs on in time) |
| **GLO + key 5–8** | **solo** track 1–4 (several solos add up) |
| **GLO + key 9 (held)** | **fill** for as long as the key is held |
| **GLO + key 10** | **fill bar**: the whole next bar plays as a fill (press again before the bar to cancel) |
| **GLO + key 16** | **tap tempo** (two taps or more) |
| **GLO + F#3** (the first black key, 2.6) | **MIDI LEARN** on / off; see [MIDI LEARN](#midi-learn-26) |
| **GLO + KNOB 1–4** | the **levels** of tracks 1–4 |
| **GLO + SELECT** | the tempo (also inside the visualiser) |

The tiles show what is heard; the *fill* and *bar* tiles light while they act.

### SAVE — song

| Control | Does |
| --- | --- |
| **SAVE + key 1–4** | play section **A–D**: playing, from the next bar (every track from its first step); stopped, it becomes the loop at once |
| **SAVE + key 1–4, several tapped while SAVE stays held** | a **quick chain** (*A B B C*…, up to 8, repeats allowed): let go and they play in turn, round and round |
| **SAVE + key 5–8** | **save the loop** into section A–D (over a used section: press the key again within 3 s) |
| **SAVE + key 9** | **new** live project: stopped, press twice within 3 s while SAVE stays held; clears the current loop and restores default sounds/tempo, preserving saved slots |
| **SAVE + key 13** | switch **loop** / **song** mode |
| **SAVE + key 14** | **SONG REC**: from the next bar, every section you play (and how long) is written into the song; again (or STOP) to end |
| **SAVE + key 16** | the **SONG** screen |

Details in [Song mode](#14-song-mode-sections-chains-songs).

---

## 10. The step sequencer: steps, nudge, locks, fills

Each track has up to **64 steps** with its own **length** (1–64) and **division** (1/32 to two bars). Tracks of different lengths loop on their own and stay in phase (polymeters). A synth step holds up to 4 notes (a chord) with a level and ratchet, a tie or a rest, an accent and a slide; a drum step holds any of the 16 sounds, each with its own level and ratchet.

There are three ways to enter steps:

1. **Live** — REC and play (see [Recording](#8-recording)).
2. **The SEQ layer** — hold SEQ, the white keys are steps (all tracks; see [SEQ — steps](#seq--steps)).
3. **The STEP page** (synth tracks) — tap **SEQ**: the step under the cursor is written from the keys, acid style:

| STEP page | Does |
| --- | --- |
| **keys** | the keys pressed together become the cursor's step (POLY: up to 4 notes; MONO, LEG and UNI: the last one); in chord mode a key writes its whole chord, CHORD+ changes included; let go of all keys and the cursor moves on · **a MIDI keyboard** too (2.6): the notes it plays together on the selected track (the track its channel plays, GLO → MIDI) become the step, as they sound; on another track's channel, stopped, the screen says *MIDI PLAYS TRACK n* (and when you record: see REC) |
| KNOB 1 **STEP** | moves the cursor |
| Hold **OCT+** + KNOB 1 **STEP** clockwise | extends after the selected note/chord with ties, one per step; stops at the pattern's end. Turning backward moves without erasing. Undo the whole gesture with EDIT + OCT−. |
| Hold **OCT−** + KNOB 1 **STEP** clockwise | writes rests into each following step; the starting step stays intact. The same end-stop, backward movement and undo apply. On STEP, the OCT buttons are painting modifiers; use another page to change playing octave. |
| Hold either **OCT** button + KNOB 2 | shifts the selected note/chord by whole octaves; selecting a tie edits its onset. Intervals and metadata stay intact; MIDI bounds limit the shift. |
| Hold either **OCT** button + KNOB 3 | moves the selected note/chord by whole steps; moving into its own tie run keeps its old end, while other moves shift the full chain. Timing, fill conditions and locks travel with the event. Stopped only; unrelated occupied destinations are protected. EDIT + OCT−/OCT+ undoes/redoes the gesture. |
| KNOB 2 **NOTE** | transposes the step (an empty step gets your last note) |
| KNOB 3 **TIME** / **LEN** | on an empty step, a TIE or a REST: **NOTE**, **TIE** (holds the previous note on) or **REST** · on a note (2.6): **LEN**, how many steps the note lasts: longer turns the steps after it into TIEs (it stops at the next step that has notes, and at the pattern's end: *NOTE AHEAD*, *END OF PATTERN*), shorter turns its last TIEs back into RESTs |
| KNOB 4 **FLAG** | **–**, **ACC** (accent), **SLD** (slide to the next note), **A+S** (both) |
| **EDIT (tap)** | clears the cursor's step and moves on |

Not while recording or armed (the keys record live then). On the drum track, the STEP page shows *DRUM TRACK*: use the DRUMS grid or the SEQ layer.

The **PATTERN** page (tap SEQ again) holds the track's **LEN**, **DIV**, **SWG** (swing) and **GATE** (how long each note lasts).

The **RECORD** page (next SEQ tap) has **SNAP** on knob 1: **TRACK** retains normal nearest-step recording, **1/8** and **1/4** snap new recordings to eighths or quarters without changing playback DIV or LEN. It is a per-track preference until power-off, applies to live recording into an existing loop on synth or drum tracks, and leaves free-take timing unchanged. Use a playback DIV that divides the requested snap grid, such as 1/16 or 1/32. A finer or incompatible requested grid shows **DIV LIMIT** and falls back to the playback grid. DIV, LEN and GATE remain available on knobs 2–4.

### Nudge (micro timing)

**SEQ + step + KNOB 4** moves a step off the grid, from −32 to +31 in 1/64 of a step: minus = early (the step plays before its grid time, inside the previous step), plus = late. Its ratchets move with it; recording and swing stay on the grid. On the drum track the whole step moves, every sound in it. A nudged step shows a dot in the corner of its tile.

### Parameter locks

A **lock** gives one sound parameter another value **for that step only** (Elektron style).

1. Turn the parameter you want on its page (for example FX → DLY send, or LFO → RATE). That becomes the **lock parameter** (before you turn anything it is ENV DEST → FLT).
2. Hold **SEQ** and hold the step.
3. Turn **PRESETS**: the first click makes the lock, one click away from the track's value; the next ones move it. The title line shows it (*lock dst 14*; *lock flt --* = no lock yet).
4. **ALGORITHM** (step still held) steps through the other parameters, wrapping round.

At the next step without a lock on that parameter, it comes back to its base value (notes still ringing follow). A knob turned on the page while a lock is in force wins: that value becomes the new base. Several locks can sit on one step (one per parameter), **24 per track**. The sound parameters of the track can be locked (and the PATTERN page's GATE); the arp, key / scale / chord, STRUM / VLEAD, voice-mode, MUTE, LEN / DIV / SWG and the global pages cannot (*NOT LOCKABLE*; *NO LOCK LEFT* when the 24 are used). A locked step shows the same dot as a nudged one; **SEQ + step + OCT−** clears both. Undo (EDIT + OCT−) is for steps and (2.7) their nudges: it does not bring locks back.

### Fill conditions

**SEQ + step + OCT+** cycles the step's condition:

| Condition | Mark on the tile | Plays |
| --- | --- | --- |
| **normal** | — | always |
| **FILL ONLY** | small **F** (top left) | only during a fill |
| **NO FILL** | **×** (top left) | always, except during a fill |

Call a fill while you play: **GLO + key 9** (as long as you hold it) or **GLO + key 10** (the whole next bar). Build a beat whose rolls, crashes and pickup notes are FILL ONLY and whose main hat is NO FILL: one finger brings the fill in and out on the bar. A skipped step is silent whole (no note, no MIDI, no ratchet, no lock). On the drum track the condition is the step's, every sound in it. STOP ends any fill. The arp and the rolls ignore fills.

### Divisions

**DIV**: 1/4, 1/8, **1/16** (default), 1/32, 8T, 16T (triplets), **1/2**, **1BAR**, **2BAR**. A 64-step track at 2BAR lasts 128 bars: drones, chord changes, slow songs.

---

## 11. Key, scales, chords and CHORD+

### Key and scale

- **SEL + any key** sets the **key of the song** for the three synth tracks. (Also SEL page → ROOT, per track.)
- **SCALE** (SEL + KNOB 2 sets all three synth tracks; the SEL page sets the selected track alone): CHR (chromatic), MAJ, MIN, DOR (dorian), MIX (mixolydian), PEN (major pentatonic), MPEN (minor pentatonic), HARM (harmonic minor), PHRY (phrygian), LYD (lydian), LOC (locrian), MEL (melodic minor), BLUES, WHOLE (whole tone), DIMHW and DIMWH (diminished, half-whole and whole-half).
- **KEYS**: **OFF** (chromatic), **SNAP** (every key rounded down to the scale), **WHITE** (the white keys walk the scale from C4, the black keys are silent).
- **TRANSPOSE** (per track): ±24 semitones.

Changing a sound (PRESETS, a user preset) never changes the key, the chord mode, the pattern or the mix of its track.

### Chord mode: one finger, one chord

Hold **SEL** and turn **KNOB 1 CHORD** on the selected synth track:

| CHORD | Notes |
| --- | --- |
| **OFF** | single notes |
| **TRIAD** | 1-3-5 |
| **7TH** | 1-3-5-7 |
| **9TH** | 1-3-7-9 (the lo-fi / R&B voicing) |
| **SUS4** | 1-4-5 |
| **POWER** | 1-5-8 (root, fifth, octave) |
| **SUS2** | 1-2-5 |
| **ADD9** | 1-3-5-9 (keeps the fifth; unlike 9TH, has no seventh) |
| **6TH** | 1-3-5-6 |
| **SHELL** | 1-3-7 (leaves the fifth out) |
| **OCTAVE** | root + 12 semitones |
| **MAJOR / MINOR** | 0-4-7 / 0-3-7 semitones |
| **DOM7 / MAJ7 / MIN7** | 0-4-7-10 / 0-4-7-11 / 0-3-7-10 |
| **DIM / AUG** | 0-3-6 / 0-4-8 |
| **HALFDIM / DIM7** | 0-3-6-10 / 0-3-6-9 |

With a chord on, **the white keys walk the scale from C4**: C4 plays the chord of the key's I, D4 the II, E4 the III… and the keys below C4 carry on downwards. TRIAD through SHELL use scale degrees (POWER uses a perfect fifth and octave). With SCALE on CHR, these degree-based chords use the minor scale. OCTAVE and MAJOR through DIM7 use fixed semitone intervals, so their quality stays the same on every key; the scale still selects each root. One finger plays the whole chord, and it is recorded as a chord.

### CHORD+: the black keys change the chord

With a chord mode on, **hold a black key while you play a white one** — or press it while the chord is held, and the chord changes under your finger:

| Black key | Changes the chord |
| --- | --- |
| **F#** | major ↔ minor |
| **G#** | adds the 7th |
| **A#** | sus4 |
| **C#** | adds the 9th |
| **D#** | an inversion |

Both octaves of black keys work, and you can hold several. The 7th and 9th come from the scale: in C major, G4 plays **G** (the V chord), **G#** makes it **G7**, **F# + G#** make it **Gm7**; on C4, F# + G# give C-E♭-G-B. What you play is recorded as it sounds. For fixed-quality chords, F# flips the actual third, A# replaces it with a perfect fourth, and C# adds a major ninth. G# adds a major seventh to MAJOR/AUG, a diminished seventh to DIM, and a minor seventh to MINOR; existing sevenths stay unchanged. At four notes, the ninth replaces the fifth. POWER and OCTAVE accept inversion only; OCTAVE raises the whole pair by an octave when both notes fit below MIDI 128.

### SEL 2: STRUM and VLEAD

Tap **SEL** twice (or SEL, then SELECT) for **SEL 2**:

| Knob | Does |
| --- | --- |
| KNOB 1 **TRN** | transpose the track, ±24 semitones |
| KNOB 2 **STRUM** | plays a chord's notes one after the other like a strummed guitar, 1–60 ms a note: right = low to high, left = high to low, centre = off. On the keys and on the chord steps the sequencer plays |
| KNOB 3 **VLEAD** | **ON**: voice leading — each chord is voiced nearest the last one, so a progression moves smoothly instead of jumping |
| KNOB 4 **LATCH** | **ON**: sustain a keyboard chord after release; the next root replaces it. Outside CHROM, black-key modifiers toggle until pressed again, including with ARP enabled and the root retained. OFF or STOP releases the latched chord. Shares ARP HOLD. Hold an existing chord plus **ARP**, or physical **SEL/SLOOP SCL**, for **700 ms** to toggle latch without restarting the chord. This also captures manually played arpeggio notes with CHORD OFF; a second button hold toggles latch off while the physical keys keep playing. |

---

## 12. Mix: levels, mute, solo, fills, tap tempo

| Where | What |
| --- | --- |
| **GLO + KNOB 1–4** | levels of tracks 1–4 |
| **TRACKS screen, KNOB 2** | the selected track's level (on a muted track: unmutes it) |
| **GLO + keys 1–4 / 5–8** | mute / solo |
| **VOICE 2 page** | PAN and MUTE of a synth track |
| **TRACKS screen, KNOB 4** | the selected track's pan |
| **GLO → DRUMS** | the drum track's LVL, its reverb and (2.5) delay sends, and its MIDI channel |
| **MASTER knob** | the output volume |

A muted track fades out in a few milliseconds and plays no new notes; its pattern runs on in time. Several solos add up.

**Tempo:** SELECT on TRACKS (or inside any layer), **GLO + key 16** tap tempo, or GLO → GLOBAL → BPM (40–240). With MIDI SYNC on, the tempo follows the master.

---

## 13. Undo, clear, save, sections, autosave

| Action | How |
| --- | --- |
| **Undo / redo** | **EDIT + OCT−** / **EDIT + OCT+**. One level: the last recording pass, erase, clear, step or pattern edit |
| **Undo an accidental load** | Firmware 2.4.16: stop playback, hold **EDIT**, press **OCT−** to restore the complete project from before the latest successful load; **OCT+** redoes the load. Saved-slot loads, stopped section selection and working-project backup restore are covered. A later sequence edit/recording takes over undo history. This snapshot lives in RAM and is lost at restart; failed/empty loads leave it intact |
| **Clear the selected track** | **hold REC** ~2 s (after 0.7 s a ring fills; let go before it is full: nothing). Undo brings it back |
| **Clear the pattern** | SAVE → TOOLS → **CLRSQ** (GO) |
| **Reset the sound** | SAVE → TOOLS → **INIT** (GO): the engine's defaults and first preset |
| **New loop** | SAVE → TOOLS → **NEW** (GO): the four tracks of your working loop back to their power-on sounds, empty patterns, 90 BPM (*NEW LOOP, A-D KEPT*: the saved sections are not touched; stop the song first in song mode; undo brings back only the last track's pattern, so save a section first if in doubt) |
| **Save the loop as a section** | **SAVE + key 5–8** (A–D) |
| **Save / load a section** | SAVE → SECTION: **SECT** A–D, **LOAD** (GO), **SAVE** (GO) |
| **Erase a saved section** (2.6) | SAVE → SECTION: **SECT** A–D, **ERASE** on KNOB 2 (GO), the song stopped: the slot is *EMPTY* again and its section key goes dark. The steps of the song that played it are taken out (the others keep their order; none left: the default song A B C D). Your working loop is not touched. No undo: save it into another slot first if in doubt |
| **Save a sound** | SAVE → USER: **SLOT** (1–32), **SAVE** (GO) |
| **Autosave** | automatic: when the transport is stopped and you have not touched anything for 2.5 s (at most every 20 s), the working project is kept in flash; at power-on SLOOP comes back exactly as you left it |

**One song on the FM-1.** The FM-1 holds one song: **four sections, A–D**, and the order they play in (the SONG screen). A **section** is a loop: the four tracks (sounds, patterns, kit, locks, nudges, fill conditions, the track filter) and the global settings of the loop (tempo, swing, key…). Your **working loop** (what plays in loop mode, kept by the autosave) is apart from them: LOAD copies a section into it, SAVE copies it into a section. A–D are not four songs. **To keep several songs**, save an editor **Backup** file per song (Song & sections tab) and restore the one you want to play. (Before 2.7 the SAVE page was called PROJECT, its slots 1–4: the same four sections.)

**Settings of the FM-1** — SYNC, MIDI OUT, IN, the lights, the menu settings, the REC screen's MODE, START and QUANT — are not part of a section: loading one or NEW never changes them.

### The SAVE pages (SAVE tapped, not on TRACKS)

| Page | KNOB 1 | KNOB 2 | KNOB 3 | KNOB 4 |
| --- | --- | --- | --- | --- |
| **PRESETS** | the sound (your favourites, then factory by kind, then user presets) | the engine (next / previous) | — | **FAV** (2.7): right adds the sound to your favourites, left removes it |
| **USER** | **SLOT** 1–32 | **LOAD** (GO) | **ERASE** (GO) | **SAVE** (GO) |
| **SECTION** | **SECT** A–D | **ERASE** (GO, 2.6) | **LOAD** (GO) | **SAVE** (GO) |
| **TOOLS** | **CLRSQ** (GO) | **INIT** (GO) | — | **NEW** (GO) |

<a id="favourites"></a>**Favourites (2.7).** On the PRESETS page, turn KNOB 4 right: the sound is added to your favourites (*FAV ADDED*), up to 16, which come first in the PRESETS list, tagged FAV, in the order you added them; the PRESETS knob reaches them in a click or two from anywhere on TRACKS or HOME. KNOB 4 left removes it. A user preset can be a favourite too. They are a setting of the FM-1, kept through power-off (not in a project, and not in an editor backup). MIDI Program Change still counts the list without them.

GO: one click shows *AGAIN: SAVE* (or LOAD…), a second click within about 1.5 s does it. Presets and sections are written to flash only while the song is stopped.

---

## 14. Song mode: sections, chains, songs

The FM-1 holds **one song**: up to **16 steps** of **4 sections, A–D**. Each section holds the four tracks (sounds, patterns, kit): SAVE → SECTION shows them. For more songs, keep one editor backup file per song.

### Make it live, by playing

**Durable saves:** stopped SAVE A–D writes flash immediately and also commits
device settings such as the 1/8 swing grid. During playback, the snapshot says
**QUEUED: STOP TO SAVE**. Stop and release sounding notes, then wait for
**SECTIONS SAVED TO FLASH** before powering off. Explicit section saves also refresh the startup working loop, including track root/scale, so a restart does not restore an older key while the background autosave is waiting. Failed writes are retried.
SONG-screen SAVE stores arrangement order; REC stores the selected section.

1. Make a loop (the verse). **SAVE + key 5**: *save A*. Change the loop (the chorus) and save it into **B** (key 6), a bridge into **C** (key 7), an ending into **D** (key 8). Saving over a used section asks for the key again within 3 s.
2. **Play the sections live: SAVE + key 1–4.** Playing: the section starts on the next bar, every track from its first step, always in time. Stopped: it becomes the loop at once.
3. **Quick chain:** keep SAVE held and tap several section keys — *A B B C*, any order, repeats allowed, up to 8 — then let go. The first plays on the next bar, then each next one after the previous has played its **pattern length** (the longest track's), round and round. The title line reads *chain A B B C*; the section playing is lit, the next one framed. A single section tap, STOP, or PLAY in song mode ends the chain.
4. **Record the song as you play it: SAVE + key 14.** From the next bar every section you play, and for how many bars, is written into the song. SAVE + key 14 again, or STOP, ends it (*SONG PARTS 5*). It is saved by itself once you stop. SONG REC records what a chain plays too.
5. **Play it back:** **SAVE + key 13** switches *loop* / *song*. In song mode **PLAY** plays the whole song and stops at the end (your loop is back afterwards).

### The SONG screen

Open it with **SAVE tapped on TRACKS** (or on DRUMS), or **SAVE + key 16**. It shows the chain and edits it by hand:

| Control | Does |
| --- | --- |
| KNOB 1 | the song step (cursor) |
| KNOB 2 | that step's section (A–D) |
| KNOB 3 | its length in bars (1–64) |
| KNOB 4 | the number of steps in the song (1–16) |
| **REC** | stores the current loop into the step's section (a used section: REC again within 3 s) |
| **SAVE** (tap) | saves the song |
| **OCT−** | loop / song mode |
| **OCT+** twice (within 3 s) | loads the step's section (unsaved work is replaced) |
| **PLAY** | play / stop |
| **SEQ** | the SEQ pages |
| **HOME** | back to TRACKS |

The song's settings can only be edited while it is stopped (*STOP FIRST*). An empty section cannot play (*EMPTY SECTION: REC*).

---

## 15. The screens

| Screen | How to get there | What it shows · what the knobs do |
| --- | --- | --- |
| **TRACKS** | tap **HOME** | the performance view: tempo, swing, transport, bar.beat; each track with its sound, steps, playhead, mute / solo / rec badges and level. Dials: KNOB 1 **swing** (global) · KNOB 2 **level** (selected track; on a muted track: unmutes it) · KNOB 3 **steps** (its length) · KNOB 4 **pan** |
| **Visualiser** | tap **HOME** on TRACKS | seven full-screen visualisers; see [The visualiser](#19-the-visualiser) |
| **Layer** | hold a layer button | 16 tiles (the white keys) and the four dials, in the layer's colour |
| **DRUMS** (grid / kit / sound / groove / rhythm) | tap **EDIT** or **SEQ** on TRACKS with the drum track | see [The DRUMS screen](#the-drums-screen) |
| **REC READY / FREE TAKE / COUNT-IN** | **REC** while stopped | the tracks, then **mode · length · start** on KNOB 1–3 (4-3-2-1 during a count-in); in a free take: the seconds and the loop it would make |
| **Hold ring** | hold **REC** | the ring of *clear track* filling |
| **SONG** | tap **SAVE** on TRACKS, or **SAVE + key 16** | the section chain; see [The SONG screen](#the-song-screen) |
| **Pages** | tap ENV, LFO, FX, SEL, EDIT, ARP, SEQ, GLO, SAVE | four colour-coded values with a graph (ENV, LFO, ARP, SEL, FX, SLICER…), or the four values in large type placed as the knobs are (1 2 / 3 4) |
| **Menu** | hold **HOME** | the settings of the FM-1 in six sections; see [The HOME menu](#20-the-home-menu-and-the-lights) |

The **title line** of every screen also carries short messages: *KEY D*, *ERASED*, *AGAIN: SAVE*, *NEXT: B*, *chain A B B C*, *lock flt 12*…

The lettering (2.6) is Terminus **Bold**, all in **capitals**: thicker strokes, easier to read on the small screen at arm's length; the layout is the same as before (same 8 × 16 cell).

---

## 16. Every page, knob by knob

Tap a button to open its first page (or the last one you used); **tap it again** for its next page, or turn **SELECT** to go back and forth. The number after the title (*PATTERN 2/2*) counts the pages SELECT goes through; tapping SEQ once more also opens the SONG screen, a screen of its own (2.7: it is no longer counted, so SEQ no longer reads *2/3*). On the drum track, pages that do not apply to it show *DRUM TRACK*.

| Button | Page | KNOB 1 | KNOB 2 | KNOB 3 | KNOB 4 |
| --- | --- | --- | --- | --- | --- |
| **ENV** | **ENV** | ATK attack | DEC decay | SUS sustain | REL release |
| | **ENV DEST** | FLT: envelope → brightness (±) | PIT: envelope → pitch (±, the 808 punch) | SHP: envelope → the engine's shape (±) | ESYNC (2.6): OFF / ON, ATK DEC REL as note values |
| **LFO** | **LFO** | RATE (Hz) | WAVE: SIN, TRI, SAW, SQR, S&H | PHS phase | FADE (fade-in time) |
| | **LFO 2** (2.6) | SYNC: OFF, 4BAR, 2BAR, 1BAR, 1/2, 1/4, 1/8, 8T, 1/16, 16T, 1/32 | TRIG: NOTE / FREE | POL: BI / UNI | — |
| | **LFO DEST** | PIT: vibrato | FLT: filter wobble | SHP: shape | AMP: tremolo |
| **FX** | **FX** (sends) | DST drive | CHO chorus send | DLY delay send | REV reverb send |
| | **FILTER** | FILT: one-knob filter of the track — left LP, right HP, centre OFF | — | — | — |
| | **SLICER** | SLCR: OFF, GATE, STUT (stutter) | PAT: pattern 1–16 | RATE: 1/8, 1/16, 1/32, 8T, 16T, 32T | DEPTH |
| | **INSERT** (2.6) | INSRT: OFF, SOFT, HARD, FOLD, FUZZ, CRUSH, PHASR, FLANG, CHOR | A | B | C (what A B C are depends on the type: see [Per track](#per-track)) |
| | **INSERT 2** (2.6) | MIX: dry / wet | — | — | — |
| | **DLY** (global) | TIME: 1/4, 1/8, 1/16, 1/32, 8T, 16T, 1/8D, 1/16D, 1/4D (2.6; at 60 BPM and below it stops at 1.49 s) | FDBK feedback | COLR colour (tone of the repeats) | MIX |
| | **REV/CHO** (global) | SIZE reverb size | DAMP reverb damping | CRT chorus rate | CDP chorus depth |
| | **REVERB** (global) | TYPE: STEREO / SPRING / ROOM / PLATE / HALL | SIZE decay | DAMP darker tail | TIME: SPRING reflection spacing, 20–80 ms; inactive on other types |
| **SEL** | **SEL** | ROOT (the track's key) | SEL: the track's scale (SEL + KNOB 2 sets all three) | QNT keys: OFF, SNAP, WHITE | CHORD: OFF, TRIAD, 7TH, 9TH, SUS4, POWER, SUS2, ADD9, 6TH, SHELL, OCTAVE, MAJOR, MINOR, DOM7, MAJ7, MIN7, DIM, AUG, HALFDIM, DIM7 |
| | **SEL 2** | TRN transpose ±24 | STRUM ±60 ms | VLEAD: OFF / ON | LATCH: OFF / ON |
| **EDIT** | **EDIT 1** | engine parameter 1 | 2 | 3 | 4 (see [the engines](#the-twelve-engines)) |
| | **EDIT 2** | engine parameter 5 | 6 | 7 | 8 |
| | **FILTER** (ANALOG only, 2.6) | TYPE: LP, BP, HP | CUT cutoff | RES resonance | KTR key tracking |
| | **VOICE** | VCE: POLY, MONO, LEG (legato), UNI (unison) | GLD glide | GLMOD: RATE / TIME | PRIO: LAST, LOW, HIGH (mono note priority) |
| | **VOICE 2** | ALLOC: ROT (rotate) / REUSE | DTUNE (unison detune) | PAN | MUTE |
| **ARP** | **ARP** | MODE: OFF, UP, DN, UPDN, RND, ORD, REPEAT, DNUP, UP+8, CONV, DIVG, PINKY, THUMB, WALK, CHORD, SHUF, ROOTALT, UPDNREP, PULSE | RATE: 1/4 … 16T | OCT: 1–4 octaves | GATE |
| | **ARP 2** | SWG swing | PROB probability of each note | HOLD: OFF / ON (latch) | ORD: NOTE / PLAY (live only), SNOTE / SPLAY (recorded steps + live) |


| **SEQ** | **STEP** | STEP (cursor) | NOTE | TIME: NOTE, TIE, REST · on a note: LEN | FLAG: –, ACC, SLD, A+S |
| | **PATTERN** | LEN 1–64 steps | DIV 1/4 … 2BAR | SWG track swing | GATE note length |
| | **SONG** | the SONG screen | | | |
| **GLO** | **GLOBAL** | BPM 40–240 | SWING (all tracks) | CLICK: OFF, REC, ON | TUNE ±50 cents |
| **FX** | **VINYL** | WOW (slow pitch drift) | TONE (darkening) | CRACKLE | — |
| | **MASTER** | DUST | DUCK | FILT (DJ filter) | ROLL (note-repeat rate) |
| | **SYSTEM** | OUT (MIDI out): KEYS / SEQ (also on MIDI 2) | SYNC: INT, USB, TRS | IN: NOTES / CLOCK / CH 1–16 (also on MIDI 2) | CPU load, with or without USB connected |
| | **DRUMS** | CH4: the drum track's MIDI channel (OFF, 1–16, default 10; the same setting as GLO → MIDI → CH4; 2.6: a setting of the FM-1) | LVL drum level | REV drum reverb | DLY drum delay (2.5): the drums into the tempo delay (FX → DLY sets its time, feedback and tone), saved with the project |
| | **TEXTURE** (2.6) | SRC: OFF, VINYL, TAPE, RAIN, WIND, RADIO, ROOM, USR1–4 | LEVEL | TONE (LP ← OFF → HP) | MOVE |
| | **TEXTURE 2** (2.6) | RATE: 1–16 bars | WHEN: PLAY / ALWAYS | DUCK: OFF / ON | — |
| | **MIDI** (2.6) | CH1: track 1's MIDI channel (OFF, 1–16) | CH2: track 2's | CH3: track 3's | CH4: the drum track's (default 10) |
| | **MIDI 2** (2.6) | IN: NOTES / CLOCK / CH 1–16 | OTHER: the channels no track has: SEL (the selected track) / OFF | OUT: KEYS / SEQ | BEND: the pitch bend's range, 1–12 semitones |
| | **MIDI 3** (2.7) | FX CH: the punch-in effects' MIDI channel (OFF, 1–16) | DR MAP: how MIDI notes play the drums (GM / 1-16) | REMOTE: the buttons over MIDI (OFF, 1–16) | CLOCK: OUT, the MIDI clock and START / STOP out on USB (OFF / OUT) |
| **SAVE** | **PRESETS** | the sound | the engine | — | — |
| | **USER** | SLOT 1–32 | LOAD (GO) | ERASE (GO) | SAVE (GO) |
| | **PROJECT** | SLOT 1–4 | ERASE (GO) | LOAD (GO) | SAVE (GO) |
| | **TOOLS** | CLRSQ: clear the pattern (GO) | INIT: reset the sound (GO) | — | NEW project (GO) |

Notes:

- **Global** pages (DLY, REV/CHO, GLOBAL, MASTER, SYSTEM, DRUMS) are the same whatever track is selected.
- **SYNC**, **MIDI** and **IN** are settings of the FM-1, kept when you load a project.
- On the **drum track**, the pages that work are SLICER, PATTERN (SEQ), the SONG screen and the global pages; its kit, level, reverb and pan are on the DRUMS kit page, and its filter is **FX + KNOB 4**.
- FM6 tracks ignore the ENV page and ENV DEST (each operator has its own envelope).
- <a id="arp-modes"></a>**ARP modes** (2.7, after Felucca 1.4 / 1.5.1). With C, E and G held: **UP** C E G · **DN** G E C · **UPDN** C E G E · **DNUP** G E C E · **RND** at random · **ORD** in the order you pressed them · **REPEAT** the last note you pressed, again and again (OCT ignored) · **UP+8** C E G C' (the first note again an octave above the top) · **CONV** C G E (from the outside in) · **DIVG** E G C (from the inside out) · **PINKY** C G E G (the top note between the others) · **THUMB** C E C G (the bottom note between the others) · **WALK** one step up or down at random, from the bottom · **CHORD** the held notes together (the first four), on every step, an octave higher each step through OCT. OCT spreads the list over 1–4 octaves; ARP 2 ORD PLAY keeps the order you played for the other modes. Until 2.6, ORD played as UP unless ARP 2 ORD was PLAY: now it always follows the order you pressed.
- <a id="kbd-hold"></a>**KBD HOLD** (2.7): ARP 2 → **HOLD ON** with ARP → **MODE OFF** holds the notes you play, without an arpeggio: let go and they go on sounding (a pad, a drone, a chord to play over with one hand). The next note you play with no key down **replaces** them (a new chord); notes played while keys are still down **join** them. The keys' MIDI note-offs wait too, so gear played from the FM-1 holds with it. **HOLD OFF** (or ARP on, another project, NEW) ends them; loading another sound ends the held notes too, but HOLD stays ON. Projects saved before 2.7 open with HOLD OFF when their arp is OFF (it did nothing then), so nothing hangs by surprise. It works for MIDI notes too. Recorded as you hear it: a held note ends where it is replaced. Up to 2.6, HOLD did nothing with the arp OFF.
- **ARP SWG** (2.7): the arp's swing also works with the sequencer stopped (the odd steps late), as it does while playing; until 2.6 it only worked while playing.
- **ARP and sounds** (2.7): while the arpeggiator runs (MODE not OFF), loading another sound (a preset or a user preset) keeps the arp: its settings (both ARP pages) and the notes it holds or latches go on with the new sound. With the arp OFF, a preset that has an arp of its own (8BIT ARP…) brings it, as before.
- **ENV SYNC** (ENV DEST → ESYNC ON, 2.6): ATK, DEC and REL become note values of the tempo, from 0 through 1/64T … 1/4, 1/2 … to 4BAR, with triplets and dotted values; at 120 BPM an ATK of 1/4 lasts half a second, at 60 a whole one. They follow the tempo as it changes. OFF: times in milliseconds, as before.
- **LFO 2** (2.6): **SYNC** locks the LFO's cycle to a note value of the tempo (RATE is then ignored; OFF: RATE, as before) · **TRIG** NOTE restarts the LFO at PHS on each new phrase (as before), FREE lets it run on · **POL** BI swings above and below the value, UNI only above it (a tremolo that only lowers the level, a wobble that only opens).
- **FILTER** (EDIT, ANALOG only, 2.6): the ANALOG engine's filter as a **low-pass** (as before), **band-pass** or **high-pass**; CUT, RES and KTR are the same values as on EDIT 2.
- On the screen, EDIT 1 and EDIT 2 carry the engine's own page names: ANALOG *OSC / FLT*, DIGITAL *OPS / MOD*, PHASE *PHS / LINE*, LOFI *CHIP / MOTN*, SAMPLE *SET / TONE*, VOICE *VOWL / TONE*, TRIO *OSC / TONE*, WHEEL *BARS / TONE*, GRAIN *GRAN / SPRY*, FM6 *OPS / PATCH*, PHYS *BODY / EXCT*, NOISE *SRC / MOVE*.
- Any sound parameter on these pages can be **locked** on a step (see [Parameter locks](#parameter-locks)), except the pattern, arp, key and voice-mode ones.

The new arp modes work with held notes and one-finger chords. **CONV** alternates the lowest and highest notes, moving inward (C–E–G–B becomes C–B–E–G). **SHUF** plays each expanded note position once in random order before reshuffling; the same note can occur at the end of one cycle and the start of the next. **ROOTALT** alternates the lowest pitch with each other note (C–E–C–G–C–B); its anchor is the lowest voiced note, including when the chord is inverted. These three modes use pitch order across the octave range. **ORD** always uses the order you pressed the keys; the separate ORD setting still controls ordering for UP, DN, and UPDN. Probability retains its existing behavior: a skipped hit does not advance the note position.

| Additional arp mode | With C–E–G–B held |
| --- | --- |
| **DNUP** | B–G–E–C–E–G, then repeat; no repeated endpoints |
| **UPDNREP** | C–E–G–B–B–G–E–C, then repeat; repeats both endpoints |
| **DIVG** | E–G–C–B, then repeat; odd-sized chords start on their middle note |
| **WALK** | Starts on C, then randomly moves one position up or down; reverses at the ends |
| **PULSE** | All held notes and selected octaves together on each hit |

These modes use pitch order across the octave range. PULSE sends each distinct pitch once per hit, so overlapping octaves do not double a note. RATE, GATE, SWG, PROB, and HOLD apply to the whole pulse. Changing modes, turning HOLD off with no keys held, STOP, panic, and section changes release every active pulse tone. Holding or latching notes after STOP lets the arp continue on its stopped clock. Internal sound still shares the eight-voice budget; MIDI can carry the full expanded chord. Recording uses the existing sequencer limit of four notes per synth step (one in mono modes); a larger pulse cannot be captured in full. PULSE starts its tones together and does not apply STRUM.

---

## 17. The sounds and the twelve engines

### The sound bank

**153 factory sounds**, every one a full patch on one of the twelve engines, all level-matched (the same LEVEL gives the same loudness). **PRESETS** browses them **by kind** — basses, keys, organs, pads, leads, plucks and bells, stabs, FX — with the kind shown next to the name; your **32 user presets** come after. A new project starts at **90 BPM** with *808 BOOM* on track 1, *RHODES* on track 2, *LOFI FLUTE* on track 3 and the 808 kit on track 4.

| Kind | Sounds (engine) |
| --- | --- |
| **Bass** | 808 BOOM, 808 DIRTY, 808 SLIDE, SUB BASS, PLUGG BASS (they slide between held notes, two octaves under the keys) · REESE, WOBBLE, ACID 303, FUNK BASS, MONO BASS (ANALOG) · FM BASS, BASS GTR (DIGITAL) · CZ BASS (PHASE) · ROUND BASS (FM6) · FAT BASS (TRIO) · WOW BASS (VOICE) · GB BASS (LOFI) · UP BASS, DEEP BASS (SAMPLE) · PLUCK BASS (PHYS) · NES BASS (LOFI) · SAW BASS (ANALOG) |
| **Keys** | RHODES, DX RHODES, WURLI, M1 PIANO, AFRO KEYS, CLAV (DIGITAL) · GRAND PNO, DUSTY PNO, LOFI KEYS (SAMPLE) · SOFT KEYS (PHASE) · TINE EP (FM6) · FM GRAND (DIGITAL) |
| **Organ** | SOUL ORGAN, GOSPEL, JAZZ ORGAN, DIRTY B3, HOUSE ORGN (WHEEL) · DRAWBARS (FM6) · ACCORDION (TRIO) · ROCK ORGAN, CHAPEL, REGGAE ORG (WHEEL) |
| **Pad** | WARM PAD, DARK STR, ATMOS PAD (ANALOG) · SAW PAD, ENSEMBLE (TRIO) · GLASS PAD (DIGITAL) · SOFT PAD (FM6) · CZ STRING (PHASE) · LOFI CLOUD, VIBE HAZE, STR CLOUD, SHIMMER, DRONE (GRAIN) · CHOIR AAH, SOUL OOH (VOICE) · BOWED MTL, CELLO BOW, GLASS BOWL (PHYS) · WIND, OCEAN (NOISE) · CZ PAD (PHASE) · VOX PAD (VOICE) · PNO FREEZE, HORN CLOUD (GRAIN) · SINE PAD (ANALOG) · PULSE PAD (TRIO) |
| **Lead** | SUPERSAW, G-FUNK LD, FAT LEAD, PAN FLUTE (ANALOG) · SYNC LEAD, HOOVER (TRIO) · TALKBOX, HARMONICA (VOICE) · GAME LEAD (LOFI) · LOFI FLUTE (SAMPLE) · FLUTE DUST (GRAIN) · NZ ARCADE (NOISE) · WAVE LEAD (LOFI) · CZ LEAD (PHASE) · ROBOT (VOICE) · PWM LEAD (ANALOG) · SQR LEAD (TRIO) |
| **Pluck & bell** | TRAP PLUCK (ANALOG) · RESO PLUCK (PHASE) · PLUGG BELL, TRAP BELL, MUSIC BOX, KALIMBA, MARIMBA, XYLOPHONE, GLOCKEN, CELESTA, STEEL DRUM, HARP (DIGITAL) · VIBES (SAMPLE) · 8BIT ARP (LOFI) · GLASS BELL, WOOD BARS, NYLON PICK (FM6) · STR PLUCK, SITAR, PHYS HARP, BELL TREE, MODAL BAR, THUMB PNO, NYLON GTR, STEEL GTR, MUTED GTR, KOTO, BANJO, TANPURA, CHIMES, BIG BELL, VIBRA BAR, STEEL PAN, WOOD BLOCK (PHYS) · 1BIT BEEP (LOFI) · CZ RESO (PHASE) · TUBE BELL, FM PLUCK (DIGITAL) |
| **Stab** | MIN STAB, MIN7 STAB, RAVE STAB, DUB CHORD (TRIO: one key plays the chord) · SYN BRASS (ANALOG) · CZ BRASS (PHASE) · BRASS SECT (FM6) · 80S POLY (TRIO) · HORN STAB, STRING STB (SAMPLE) · CHIP CHORD (LOFI) · POWER STAB (TRIO) |
| **FX** | SCRATCH (scratch, backspin, rewind across the keys) · GM KIT (SAMPLE) · HAND DRUM, MEMB TOMS, TABLA, CONGA, TIMPANI (PHYS) · RAIN, NZ METAL, VINYL, HISS, RISER, NZ SNARE, BITCRUSH, RADIO (NOISE) · CHIP NOISE (LOFI) |

To pick an engine directly: SAVE → PRESETS → **KNOB 2**. To start from the engine's defaults: SAVE → TOOLS → **INIT**.

### The twelve engines

Each engine's eight parameters are on **EDIT 1** (KNOB 1–4) and **EDIT 2** (KNOB 1–4), tap EDIT once and twice; the screen shows them under the engine's own page names (for example DIGITAL: *OPS*, *MOD*). A dash is an unused knob.

| Engine | Sound | EDIT 1: KNOB 1 · 2 · 3 · 4 | EDIT 2: KNOB 1 · 2 · 3 · 4 |
| --- | --- | --- | --- |
| **ANALOG** | virtual analogue: two oscillators, filter | WAVE (SAW, SQR, TRI, SIN, PWM) · DTN detune · MIX · NOIS noise | CUT cutoff · RES resonance · DRV drive · KTR key tracking |
| **DIGITAL** | 4-operator FM | ALG algorithm 1–8 · R2 · R3 · R4 (operator ratios) | IDX FM amount · MDEC modulator decay · FB feedback · — |
| **PHASE** | phase distortion (CZ style) | WAVE · WAVE2 (a second wave, or –) · DCW (the distortion amount) · ENV (its envelope) | DTN detune · LINE: MIX / RING · SUB sub-oscillator · — |
| **LOFI** | 8-bit chip | CHIP: 4BIT, 4B/2, 8BIT, 1BIT, STEP · WAVE: PLS, TRI, SAW, NOIS, WRAM · DUTY (pulse width / wave table) · CRSH crush (DCY on STEP) | SWP pitch sweep · VIB vibrato · ARP: OFF, OCT, MAJ, MIN plus all SEL chord shapes (chip arpeggio) · TONE |
| **SAMPLE** | sampler | SET: PIANO, BASS, VIBES, HORNS, STRGS, FLUTE, SCRCH, PERC, **USR1–USR4** · TUNE ±24 · BITS (bit reduction) · LOOP: OFF / ON | CUT (tone) · — · DRV drive · — |
| **VOICE** | formant voice (choirs, talkbox) | VOWL vowel · VOWL2 second vowel · TALK (time from one to the other) · SHIFT formant shift ±12 | BUZZ · BRTH breath · Q · RAND |
| **TRIO** | three oscillators | WAVE (16 combinations) · INT2 · INT3 (intervals of oscillators 2 and 3, ±24) · DTN | MODE: LP, BP, HP, NOT (filter type) · CUT · RES · PW pulse width |
| **WHEEL** | tonewheel organ | REG (16 drawbar registrations: FLUTE, MELLO, HOLLW, SMOOT, 3BAR, BLUES, GOSPL, ROCK…) · SUB · BODY · TOP (drawbar groups ±8) | PERC: OFF, 2ND, 3RD, 2SOFT, 3SOFT, 2SLOW, 3SLOW · CLICK · DRV · ROTR: OFF, SLOW, FAST (rotary speaker) |
| **GRAIN** | granular | SRC (any sample set, USR1–USR4 too) · POS position · SIZE grain size · DENS density | PTCH ±24 · SPRD spread · RAND randomness · TONE |
| **FM6** | six-operator FM, DX7 patches | ALG: PAT (the patch's) or 1–32 · FB feedback · MLVL modulator level (brightness) · MRAT modulator ratio | MEG modulator envelopes slower / faster · VMOD velocity → brightness · DTUN carrier spread · PTCH: F1–F8 factory, B1–B27 your bank |
| **PHYS** | physical models: strings, bars and bells, drum heads, sympathetic strings (DaisySP and Rings, as Felucca 1.0 ported them; 3 voices a track, SYMP 2) | MODEL: MODAL, STRNG, MEMB, SYMP · STRC structure (MEMB: HARM, SYMP: CHRD the sympathetic strings' chord) · BRIT brightness · DAMP | POS strike / pluck position (SYMP: SYMP, how much they ring) · ACC accent · BOW bowing (MEMB: BEND, SYMP: BUZZ) · EXC the exciter in the mix |
| **NOISE** | noise, from analogue to digital: colours, crackle, shift registers, metallic tones | MODE: ANLG, DUST, LFSR, META · COLR white → pink → brown · FREQ · RES | TRK key tracking · DENS impulses (LFSR, META: LEN, the register) · DRFT drift · CRSH crush (LFSR, META: CLK, the clock above the key) |

The **SAMPLE** sets are free recordings (CC0: Versilian Studios VSCO-2 CE and VCSL, Sonic Pi), coloured like a record through an old sampler. **GRAND PNO** is a Steinway recorded note by note; long notes fade as on the real one.

### FM6 and DX7 patches

**FM6** is msfa, the synthesis core of Dexed, as Felucca 1.0 ported it: six operators with their own four-stage envelopes, keyboard scaling, velocity, ratio or fixed frequency and detune; 32 algorithms, feedback, LFO, pitch envelope; six voices per track. The eight EDIT values above are **macros on top of the patch**; the track's FLT moves MLVL (so LFO → FLT still brightens it), SHP the feedback, PIT the pitch.

Edit full patches in the web editor's **FM6** panel (Sound page, on an FM6 track): every operator, **Send to track**, **Store in bank** (B1–B27), **Import SysEx** (a DX7 voice or a 32-voice bank: thousands of DX7 and Dexed patches play on the FM-1; or drop the .syx file on the panel) and **Export SysEx**. **Store cartridge in bank** (2.5) puts a whole 32-voice cartridge on the FM-1 at once: the first 27 voices are ticked (or tick the ones you want, up to 27), and one write puts them in B1, B2… in order; the editor asks before replacing, and the rest of the bank stays. Stop the song first. Before storing you can choose to **also add them to the user presets** (in the free slots, or from a slot you pick): each voice gets a user preset with its name that plays its B slot, so on the FM-1 you pick it by name with **PRESETS** or **SELECT**, like any sound; the editor says which user presets it would replace. (A user preset keeps PTCH, not the patch: the voice itself stays in the bank.) Then pick them on the FM-1 with **PTCH** (EDIT page 2: B1–B27 after the factory F1–F8). A project keeps the track's **PTCH**, not the patch itself: store an edited patch in the bank to keep it.

---

## 18. Effects: master, punch-in, per track

On the drum track, the first FX page is **LEVEL / PAN / DLY / REV**. DLY and REV are the drum bus's delay/reverb send percentages; synth sends remain independent. The second page edits the drum filter. **INSERT 2** has MIX / TYPE / the insert's first two sound controls, on both drums and synths.

**FX → LOFI:** knob 1 selects VINYL, TAPE, VHS or CD. Knobs 2–4 are WOW/DARK/DAMAGE, FLUTTR/DARK/DRIVE, WARBLE/DARK/DAMAGE, or RATE/LATCH/MIX respectively. All three amounts zero bypass processing; mode alone does not activate it. CD repeats very short fragments from the existing buffer, rather than phrase-length skips. These settings save with sections/projects. Held VINYL temporarily uses its own vinyl settings and restores the saved character after release.


### Signal path

Each synth track: engine → **drive** (DST) → **INSERT** (2.6) → **SLICER** → **FILTER** (the track's own) → level (and DUCK) → pan and **sends** to the stereo chorus, the tempo delay and the reverb. The drum track has its own INSERT (2.6), slicer, filter, level, pan, reverb send and (2.5) delay send. Then the whole mix, with the effect returns (and, 2.6, the **TEXTURE** bed): **DUST** → **punch-in effects** → **FILT** (the DJ filter) → **MASTER** volume → limiter (2.6: it looks one block, 0.7 ms, ahead: a loud hit is already turned down when it comes, never pushed into the output's soft clipper).

### Master: DUST, DUCK, FILT, ROLL

On FX + KNOB 1–3, or GLO → MASTER:

| | Does |
| --- | --- |
| **DUST** 0–100 % | the mix through an old sampler and a record: drive into a soft clip, a lower sample rate (down to ~11 kHz), fewer bits (down to 8), a low-pass closing to ~3 kHz, and, while the transport plays, a little hiss and crackle (a stopped SLOOP is silent) |
| **DUCK** 0–100 % | every kick pumps the synth tracks down and back over an 1/8 note: the sidechain sound, in time at any tempo |
| **FILT** | a DJ filter: left of centre a low-pass closing, right a high-pass opening, centre OFF; it glides (no zipper noise) |
| **ROLL** | the note-repeat rate of ARP + key |

### Reverb spaces

On **FX → REVERB**, try **ROOM** for short reflections, **PLATE** for a bright,
dense tail, or **HALL** for a larger space. **STEREO** retains the original sound.
**SPRING** has two unequal dispersive paths for a wider, textured spring tail.
Knob 2 SIZE controls decay and knob 3 DAMP removes high-frequency energy.
Only SPRING uses knob 4 TIME, independently setting reflection spacing.
Use each track's REV send to choose how much goes into the shared space.
Types save with projects and sections. Switching fades/clears the old tail;
FREEZE postpones the change until released. ROOM/PLATE/HALL are algorithmic
voicings, not measured models of specific rooms or hardware.

### Punch-in effects

**HALF** now slows the running musical transport to 0.5x while preserving note
pitch. Release resumes normal speed from the current position. Saved BPM stays
unchanged; beat-synced controls follow the slower clock, while free-running voices,
ordinary envelope times and effect tails remain real-time. HALF is available with
internal clock only; external synchronization shows "HALF: INT only".

Hold multiple keys to combine one existing punch effect with one LOW/HIGH/PHONE
filter and GATE. HALF can accompany these. Within a conflicting family, the last
pressed key wins; releasing it restores the previous held choice. Physical keys
have priority over MIDI within each family. USB and TRS holds are independent.
FREEZE stays exclusive with the filter/gate stages. Leaving the FX layer clears
physical holds; STOP clears all holds. Remote control keeps its single-effect
contract.


**FX + white key**: the effect runs on the whole mix while the key is held and lets go cleanly when you release it. Loops and the gate are locked to the tempo and start on the grid. **Over MIDI (2.7):** set GLO → MIDI 3 → **FX CH**, and that channel's notes F3 … G5 play the same 27 effects (see [the MIDI settings](#all-the-midi-settings-glo--midi-midi-2-26-and-midi-3-27)).

| Key | Effect | Key | Effect |
| --- | --- | --- | --- |
| 1 · F3 | loop 1/4 | 9 · G4 | low-pass sweep |
| 2 · G3 | loop 1/8 | 10 · A4 | high-pass sweep |
| 3 · A3 | loop 1/16 | 11 · B4 | phone |
| 4 · B3 | loop 1/32 | 12 · C5 | bit crush |
| 5 · C4 | stutter (1/16 triplets) | 13 · D5 | alias (sample-rate drop) |
| 6 · D4 | reverse | 14 · E5 | gate 1/16 |
| 7 · E4 | tape stop | 15 · F5 | echo (dotted 1/8) |
| 8 · F4 | HALF: transport 0.5x, same pitch | 16 · G5 | tape wobble |

**FX + black key** (2.6): eleven more, on the black keys from the lowest F# to the highest.

| Key | Effect | Key | Effect |
| --- | --- | --- | --- |
| F#3 | **KILL LOW**: the lows gone (below ~250 Hz, 24 dB/oct, DJ style) | F#4 | **FLANGER**: a jet sweep over a bar, left and right apart |
| G#3 | **KILL MID**: the mids gone (~250 Hz – 2.5 kHz) | G#4 | **AUTO-PAN**: the mix in mono, left and right once a beat |
| A#3 | **KILL HIGH**: the highs gone (above ~2.5 kHz) | A#4 | **FREEZE**: the mix poured into the reverb and held there for as long as you hold the key; let go and it dies away with the reverb's SIZE |
| C#4 | **REWIND**: the record pulled back: it slows, stops and spins backwards faster and faster (about a third of a second of the past), then silence until you let go | C#5 | **LOOP 8T**: loop 1/8 triplet |
| D#4 | **DOUBLE TIME**: double the musical tempo while held (pitch unchanged) | D#5 | **VINYL**: momentary wow, tone and crackle |
| | | F#5 | **FADE OUT**: the mix fades to nothing over two beats |

The screen shows the 27 keys as a keyboard: the white keys in four rows of four, as on the other layers, the black keys above them, each between the two white keys it sits between on the FM-1 (the one after a row's last white key at the right end of that row). The effect held is lit white.

### Texture (2.6)

**GLO → TEXTURE** (and TEXTURE 2): a looping bed under the whole mix, the glue of lo-fi and ambient tracks, without taking a track. It goes into the dry mix (not the sends), before DUST, the punch-ins and the master filter.

| Knob | TEXTURE | TEXTURE 2 |
| --- | --- | --- |
| 1 | **SRC**: OFF, **VINYL** (hiss, rumble, crackle), **TAPE** (dark hiss, drifting), **RAIN** (a bed and droplets), **WIND** (resonant gusts), **RADIO** (bursts of static), **ROOM** (room tone), **USR1–USR4** (your sample slot, its first sound looped at its own pitch) | **RATE**: how long MOVE's breath takes: 1, 2, 4, 8 or 16 bars of the tempo |
| 2 | **LEVEL** | **WHEN**: PLAY (it fades in and out with the transport) / ALWAYS |
| 3 | **TONE**: left a low-pass, right a high-pass, centre open | **DUCK**: ON, the kick pumps it too (GLO → MASTER → DUCK) |
| 4 | **MOVE**: how much it breathes over RATE (its level and brightness) | — |

The texture is saved with the project, and each song section (A–D) has its own. The generated sources use no memory; at SRC OFF nothing runs.

### Per track

- **FILTER** (FX → FILTER, or **FX + KNOB 4** for the selected track): one knob, left a low-pass, right a high-pass, centre off. It stays when you change the sound, can be locked on a step, and is saved with the project.
- **Sends** (FX page): DST drive, CHO chorus, DLY delay, REV reverb. The delay and the reverb / chorus settings are global (DLY and REV/CHO pages). The delay's TIME includes dotted 1/8 and 1/16.
- **FX → REVERB → TYPE**: STEREO keeps the existing reverb (default); SPRING adds
  a dispersive spring sound. SIZE/DAMP adjust either model. Model changes fade
  the old tail out before clearing/reusing its buffers; no overlapping tails.
  A model change requested during FREEZE waits until FREEZE releases. The model
  is saved with projects and sections; older projects select STEREO.

- **SLICER** (FX → SLICER): a tempo gate (GATE) or stutter (STUT), 16 patterns, 1/8 to 32T, with a depth.
- **INSERT** (FX → INSERT and INSERT 2, 2.6, after Felucca 1.5): one effect on the track alone, right after its drive; the drum track has one too, on the whole kit and its sends. MIX sets dry / wet. A, B and C change with the type:

| Type | A | B | C |
| --- | --- | --- | --- |
| **SOFT** (soft clip), **HARD** (hard clip), **FOLD** (wavefolder), **FUZZ** | DRIVE | TONE | LEVEL |
| **CRUSH** | BITS 1–16 | RATE (the sample rate, 689 Hz – 44 kHz) | LPF |
| **PHASR** (phaser), **FLANG** (flanger), **CHOR** (chorus) | RATE | DEPTH | FDBK feedback |

  A, B, C and MIX can be locked on a step. TYPE OFF (or MIX at 0) costs nothing: the track sounds exactly as without it.

---

## 19. The visualiser

On the TRACKS screen, **tap HOME**: the whole screen becomes a visualiser of what SLOOP plays.

| Control | Does |
| --- | --- |
| **SELECT** | the next / previous style (its name shows a second; the last one is kept with the settings) |
| **HOME**, or any page button | closes it |
| keys, PLAY, REC, layers | work as ever (a held layer shows its screen, then the visualiser comes back) |
| KNOB 1–4 | nothing (the tempo is GLO + SELECT meanwhile) |

| # | Style | Shows |
| --- | --- | --- |
| 1 | **OSCILLOSCOPE** | the mix waveform, standing still |
| 2 | **SPECTRUM** | 32 frequency bands with falling caps |
| 3 | **SPECTROGRAM** | scrolling frequency history |
| 4 | **VU METERS** | tracks 1–4 and master, with peak holds |
| 5 | **NOTE TRAILS** | played and held synth pitches with high-contrast drum lanes; delay tails excluded |
| 6 | **GROOVE** | grid, swing, microtiming, hit levels and ratchets |
| 7 | **SONG JOURNEY** | song/quick-chain order, current section and remaining bars |

The visualiser sees the mix **as if MASTER were all the way up**: with MASTER turned down, even to 0, it still moves at full size. Drawing runs in the UI loop using existing audio/transport snapshots, without adding per-sample rendering to the audio path. See the [seven-style screenshot gallery](docs/firmware/VISUALIZERS.md).

---

## 20. The HOME menu and the lights

**Hold HOME** (0.7 s) to open the menu; hold it again to close it. The menu is in six sections:

| Section | KNOB 1 | KNOB 2 | KNOB 3 | KNOB 4 |
| --- | --- | --- | --- | --- |
| **SCREEN** | COLOR / your theme | ZOOM | KNOB ACCEL | — |
| **LIGHTS** | LIGHTS | KEYS | NOTES | — |
| **AUDIO** | SPEAKER LOWCUT | USB AUDIO | USB RATE | — |
| **MIDI** | MIDI SCALE: OFF / KEYBOARD | — | — | — |
| **TIMING** | SWING GRID: STEP / 1/8 | — | — | — |
| **SYSTEM** | 2ND CORE: ON / OFF | HARDWARE CALIBRATION | MIDI LEARN CLEAR | ABOUT |

USB RATE and enabling the second core take effect at the next power-on.
USB SERIAL is available only in builds with the developer console enabled.

| Control in the menu | Does |
| --- | --- |
| **SELECT** | the previous / next section |
| **KNOB 1–4** | set the section's rows directly (each row shows its knob's colour) |
| **PRESETS** | moves the cursor |
| **OCT+** | steps the cursor's setting round, or opens it (CALIBRATION, ABOUT) |
| **OCT−** | closes (from ABOUT: back to the section) |
| **hold HOME** | leaves the menu |

All of these are **settings of the FM-1**, kept when you load a project or start a new one.

### Lights

- **LIGHTS** — every button glows at that level, so its label can be read in the dark (on a black FM-1 the labels are unreadable unlit). What is on — the page, PLAY, REC, an octave — stays at full light and still blinks.
- **KEYS** — the Cs, every white key, or every key glow at the LIGHTS level too (KEYS turns LIGHTS on at LOW if it was off).
- **NOTES** — ON: the notes sounding on the selected synth track light their keys, played live or by the sequencer (each for at least a tenth of a second, so short sequencer notes show too). The drum track always lights its hits. Works on every page and in every layer.
- **Always:** PLAY flashes on every beat; REC blinks when armed and is solid while recording; the button of the page shown is lit; in a layer, keys 1, 5, 9, 13 glow dimly and what is on is fully lit; OCT− / OCT+ light when the octave is off 0; a locked layer's button blinks.

### Hardware calibration

If a button or knob does the wrong thing, HOME menu → SYSTEM → **HARDWARE CALIBRATION** (or hold **OCT− + OCT+** while switching on): press each button and turn each knob to the right as the screen asks. 30 s without input cancels it and keeps the old setup.

---

## 21. MIDI

### Where MIDI comes in

- **The MIDI IN jack** (3.5 mm TRS on the FM-1): a keyboard or pad controller with a MIDI output, through a TRS-to-DIN MIDI adapter. If nothing plays, try the other type of adapter (type A / type B).
- **USB**: a computer or phone (a DAW, a MIDI routing app) or a USB MIDI host box. A USB keyboard plugged straight into the FM-1 cannot work: both are USB devices, and a USB link needs a host.

Both work at once.

### All the MIDI settings: GLO → MIDI, MIDI 2 (2.6) and MIDI 3 (2.7)

Every MIDI setting is on these pages of GLO (tap GLO until **MIDI**). They are settings of the FM-1: kept through power-off, not changed by a project, and in the web editor's backups.

| Page | KNOB 1 | KNOB 2 | KNOB 3 | KNOB 4 |
| --- | --- | --- | --- | --- |
| **MIDI**: which channel each track uses, in and out | **CH1** track 1 (default 1) | **CH2** track 2 (2) | **CH3** track 3 (3) | **CH4** the drum track (10) |
| **MIDI 2**: how MIDI comes in and goes out | **IN** | **OTHER** | **OUT** | **BEND** |
| **MIDI 3** (2.7): the punch-in effects, the drums, the buttons over MIDI, the clock out | **FX CH** | **DR MAP** | **REMOTE** | **CLOCK** |

- **CH1–CH4:** OFF or 1–16. A track's channel plays it (notes, CCs, sustain, pitch bend, Program Change) and its keys and sequence go out on it. **OFF**: nothing in, nothing out for that track. Two tracks never share a channel: turn a channel onto one another track has and it goes on to the next free one, and the screen says whose it was (*CH 3 = TRACK 3*). Changing a channel ends whatever was still sounding on it, here and on the other gear. CH4 is also on GLO → DRUMS (the same setting).
- **IN:** **NOTES** (notes, CCs and the clock: normal) · **CLOCK** (only the clock and START / CONTINUE / STOP; every note and CC ignored, except the REMOTE CCs when REMOTE is on) · **CH 1 … CH 16** (only that one channel, always on the selected track; CH1–CH4 and OTHER are set aside, the clock still followed). Opening GLO → MIDI while IN is not NOTES says so on the screen (*IN CH 5: SEL TRK ONLY*, *IN = CLOCK: NO NOTES*).
- **OTHER:** what a channel that no track has does: **SEL** plays the selected track (set your keyboard to channel 4 and it follows ALGORITHM) · **OFF** ignores it (a sequencer's channels for other gear on the same cable).
- **OUT:** **KEYS** only what you play on the keys goes out · **SEQ** the sequencer, the arp and the rolls go out too.
- **FX CH** (2.7): OFF (the default) or 1–16. On that channel the notes **F3 … G5** (MIDI 53–79) are the FM-1's 27 keys with **FX** held: a note held holds its [punch-in effect](#punch-in-effects), its release lets it go (white keys: effects 1–16, black keys: 17–27, as on the FM-1). A pad controller, a keyboard or a DAW can play and automate them. That channel only plays effects (choose one no track uses); it works with IN = NOTES or CH n, not with IN = CLOCK. A key of the FM-1 (FX + key) takes over at once.
- <a id="dr-map"></a>**DR MAP** (2.7): how the notes reaching the drum track (on CH4, or on IN CH n with the drum track selected) pick the 16 drum sounds. **GM** (the default, as before): the General MIDI notes, for DAWs and MIDI files: 36 kick, 35 kick 2, 38 snare, 39 clap, 42 hat, 46 open hat, 44 pedal hat, 37 rim, 40 snare 2, 43 low tom, 48 high tom, 49 crash, 51 ride, 70 shaker, 63 conga, 56 cowbell (any other note plays the nearest of them). **1-16**: any 16 notes in a row play the 16 sounds in order, from note 36 (KICK, KICK 2, SNARE, CLAP, HAT, OPEN HAT, PEDAL, RIM, SNARE 2, LOW TOM, HI TOM, CRASH, RIDE, SHAKER, CONGA, COWBELL), and again every 16 notes: a keyboard or a row of pads plays the whole kit wherever it starts. What the drum track sends on MIDI OUT stays GM.
- <a id="remote"></a>**REMOTE** (2.7): OFF (the default) or 1–16. On that channel, **CC 102 … 115** press the FM-1's 14 buttons from a controller or a DAW: a value of 64 or more holds the button down, below 64 lets it go (a controller's button sends 127 then 0: a tap; held: a hold). Everything works as on the panel: taps, holds and their layers, combinations (a button held + a key), OCT− / OCT+. CC 102 FX · 103 SEL · 104 ENV · 105 LFO · 106 EDIT · 107 GLO · 108 HOME · 109 SAVE · 110 ARP · 111 SEQ · 112 PLAY · 113 REC · 114 OCT− · 115 OCT+. Choose a channel no track uses; its other messages work as usual (notes, other CCs, MIDI LEARN).
- <a id="clock-out"></a>**CLOCK** (2.7): OFF (the default) or **OUT**. OUT, the FM-1 is the **clock master** on USB: it sends the MIDI clock (24 pulses a beat, at its tempo, also while stopped so the other gear knows the tempo), **START** when its sequencer starts (after a count-in, when it starts) and **STOP** when it stops. A DAW, a drum machine or another groovebox set to follow MIDI clock then plays in time with SLOOP and starts and stops with it. It works with SYNC = INT only: a FM-1 that follows a clock (SYNC USB or TRS) never sends one (no loop through a computer). The FM-1's TRS jack is an input: the clock goes out on USB. A setting of the FM-1, kept through power-off.
- **BEND:** the pitch bend's range, 1–12 semitones (default 2).

IN and OUT are also on GLO → SYSTEM, next to SYNC (the clock), where they were before 2.6.

**Examples.** A keyboard on channel 1 plays track 1; select another track and play it from channel 4 (OTHER = SEL). An external sequencer with drums on 10, bass on 2, pad on 11, lead on 8: CH1 = 2, CH2 = 11, CH3 = 8, CH4 = 10, OTHER = OFF. A KeyStep Pro whose other channels drive other gear: IN = CH n, and that channel plays the selected track.

### MIDI out, clock in (GLO → SYSTEM)

| Knob | Setting | Choices |
| --- | --- | --- |
| KNOB 1 | **OUT** (MIDI out) | as on MIDI 2 |
| KNOB 2 | **SYNC** (clock in) | **INT**: SLOOP's own tempo · **USB** or **TRS**: follow that MIDI clock |
| KNOB 3 | **IN** | as on MIDI 2 |
| KNOB 4 | **CPU** | the processor load, with or without USB connected |

- **MIDI out:** each track on its channel (CH1–CH4). With **SEQ**, set a DAW track to record the FM-1's MIDI and you get the pattern as notes, or let SLOOP drive another synth. Every note is ended, STOP ends whatever was still on, and notes that came in are never sent back (no MIDI loop).
- **Clock in:** START plays from the top, CONTINUE carries on, STOP stops; the tempo follows the master pulse by pulse (24 a beat), so SLOOP cannot drift. When the clock stops for half a second, PLAY on the FM-1 plays at its own tempo again. A MIDI START during the REC count-in starts and records at once.

The displayed external BPM and tempo-dependent effects use an average of up to
eight beats with a small hysteresis, avoiding jitter between neighboring BPM
values. A change of at least five BPM resets the average promptly. Phase still
follows the actual clock pulses; this smoothing does not quantize incoming notes.
- Switching IN, or a channel, releases any note still held: nothing is left hanging.
- **Bluetooth MIDI** is not supported: SLOOP never switches the radio on.

### MIDI CCs (2.5)

The knobs of a MIDI controller set the sound (the standard CC map of Felucca 1.1.5). A CC acts on the track its channel plays, as the notes do (each synth track's channel, the drum channel the drum track, the others the selected track), and sets its parameter as a knob would: 0–127 over the parameter's range, 64 the middle of PAN and FILTER. It changes the sound's value itself (as a knob, so saving keeps it); the screen and the web editor follow.

| CC | Sets |
| --- | --- |
| 5 | GLIDE |
| 7 | LEVEL (on the drum channel: GLO → DRUMS → LVL) |
| 10 | PAN |
| 71 | the engine's resonance (RES or Q: ANALOG, TRIO, VOICE, NOISE; the others ignore it) |
| 72 · 73 · 75 | release · attack · decay |
| 74 | (2.7) the engine's own cutoff or brightness, dark to bright: ANALOG / SAMPLE CUT, TRIO CUT, PHASE DCW, LOFI / GRAIN TONE, FM6 MLVL, PHYS BRIT, NOISE FREQ. DIGITAL, VOICE, WHEEL and the drum track: the track's FILTER (64 off, lower a low-pass, higher a high-pass), as before |
| 91 · 93 · 94 | the reverb, chorus and delay sends (on the drum channel 91 and 94 are GLO → DRUMS → REV and DLY) |

The fork also accepts **CC1** for transient vibrato on synth tracks; CC120/121/123
clear modulation. Sustain and pitch bend are described below.

### Scale-aware grids and incoming MIDI (Merthsoft.13)

The 4×4 grids correspond to the FM1's sixteen physical white keys, in order.
Synth EDIT shows a key-signature guide with physical-key hints:
F# in E minor, Eb in C minor. The guide does not remap playing or erasing.
Held SEL displays actual mapped pitches or their chords; each tile's small
**root F**, **root G**, etc. hint shows the root selected by pressing that key while
SEL stays held. Selecting a song root remains a literal physical-key action.

Set **KEYS = WHITE** for successive scale degrees (C4 is the selected root): in
C minor, the physical E key plays E♭ and its tile reads **Eb4**. SNAP rounds down
to the scale; OFF keeps chromatic playing; SEL marks outside-scale notes dimly.
The display uses flats/sharps appropriate to the selected key, rather than always
using sharps. This does not automatically enable scale mapping or change the LED
scale guide's physical pitch classes.

Hold **HOME**, turn **SELECT** to **MIDI SCALE**, then turn **knob 1** right to set
**MIDI SCALE = KEYBOARD**; left restores **OFF**. It is a persisted device setting,
off by default. KEYBOARD applies the receiving synth's keyboard pitch mapping,
including KEYS mode, root, scale, octave and transpose, to incoming USB and TRS
notes across the MIDI range. In WHITE mode black input keys are silent; CHROM
keeps literal roots. It does not generate chord voicings or treat MIDI black keys
as chord modifiers. Drums and sampler GM kits remain literal.

Use OFF when a sequencer or the phone already sends the intended pitches. Every
incoming note remembers its sounded pitch and destination until release, so changing
settings while holding it cannot strand a note. Many-to-one mappings release only
when their last MIDI owner releases. USB reset releases USB-owned notes and retains
TRS owners. Up to 64 incoming synth key owners are tracked; excess note-ons are
ignored until a slot is free. Keyboard and sequencer ownership retain their existing
rules. See [implementation details](docs/firmware/SCALE-MIDI-GRIDS.md).

### Live vibrato, tremolo and modulation wheel (Merthsoft.14)

Select a synth track, then hold **LFO** for gentle vibrato or **ENV** for tremolo.
Tap **OCT+** while that panel is held or locked to apply its parameters to the track's
saved shared LFO. The temporary effect stops, and octave does not change. Rate,
waveform and beat division transfer; tremolo depth is exact, vibrato depth rounds
to PIT steps. Other destination depths remain but share the new timing/waveform.
Save the project normally to retain it after power-off.
A temporary panel appears after 140 ms. **Knob 1** adjusts speed, **knob 2** depth,
**knob 3** waveform (sine / triangle / saw / square), and **knob 4** beat sync.
SYNC offers OFF, 1/4, 1/8, 1/16, 1/32, 8T, 16T, 1/2, 1BAR and 2BAR.
OFF uses knob 1 Hz; turning knob 1 also returns to OFF. Synced cycles follow the
transport phase while playing, including external clock; while stopped they run at
the current BPM. The saved patch LFO remains separate.
Keyboard scale lights and backlight preferences remain unchanged while held.
Release the button to remove the effect and return to the previous screen. Quick
LFO/ENV taps still open their normal pages. Keep playing the keyboard while held.

Only the synth selected when the hold starts is affected. Switching tracks cancels
it. The settings are separate from the saved LFO and envelope, carry across holds,
and reset at reboot. Both buttons can be held together; the LFO panel takes knob
priority. **Hold LFO/ENV + tap HOME**, then release the button, to lock its panel
and effect. LOCK appears; notes and knobs remain playable. HOME unlocks. Another
function button unlocks on the first press; press again to open its normal page.
STOP, track panic, menu entry or a selected-track change clears the locked effect.
Unlock before selecting another track with the normal buttons. One panel can lock
at a time, and locks are not saved in projects or across reboot.
Vibrato defaults to about 5 Hz and +/-12.5 cents, reaching +/-50 cents at full depth;
tremolo defaults to about 4 Hz at 38% depth. Drums are unaffected.

Held notes, latched chords and arps continue without retriggering. Physical vibrato
has priority over incoming CC1 for that synth; release restores the latest wheel
value. CC1 uses the saved track LFO rate/wave/fade and normal channel routing.
STOP, track panic and CC120/121/123 reset modulation; menu entry cancels hardware
holds. USB reset/detach clears USB-owned wheel values while retaining TRS values.
Physical modulation is not sent as outgoing MIDI or recorded as sequencer notes.

The held panels show a modulation waveform with a moving phase marker: pitch
offset in cents for vibrato, volume percentage for tremolo. Track/key context and a
compact scale-note guide stay visible. The display does not remap the keyboard.

Android Perform's **Mod wheel** strip sends CC1: slide right to increase and release
to send zero. Touch cancellation, track/tab change, stop and backgrounding also reset
it. The strip and XY macro take turns owning CC gestures. With another MIDI destination,
CC1 uses that instrument's own mapping. Controller input through Android does not yet
forward CC1; an external wheel connected directly to FM1 does work.
**Sustain and pitch bend (2.6).** The **sustain pedal** (CC 64, on the track its channel plays): while it is down, a note you let go keeps sounding until the pedal comes up (the arp keeps it, a recording holds it as you hear it; a note played again meanwhile ends with its key). **Pitch bend** bends the track its channel plays, smoothly, by up to **BEND** semitones (GLO → MIDI 2: 1–12, default 2); the drum track ignores it. Other unmapped CCs are ignored; CC1 retains the fork’s transient vibrato behavior.

**Program Change (2.6, after Felucca 1.5.1 and @renebohne's PR #179).** A Program Change on a track's channel loads its sound: on a synth track, number *n* is the *n*-th sound of the PRESETS list (0 the first: 808 BOOM; the 153 factory sounds by kind, then your user presets in order), on the drum channel kit *n*. A number past the end is ignored. The screen says *T2 PROG 5*. IN = CLOCK ignores it; IN = CH n sends it to the selected track.

### MIDI LEARN (2.6)

Any knob of your controller onto any parameter of a track (after Felucca 1.5):

1. **GLO + F#3** (the first black key): *LEARN: TURN A KNOB*; the top line shows **LEARN**.
2. Turn the knob of SLOOP you want (any page of the track, or LEVEL / PAN on TRACKS): *T1 CUT: SEND A CC*.
3. Move the knob on your controller: *CC 21 = T1 CUT*. Go on with other knobs, as many as you like.
4. **GLO + F#3** again: *LEARN DONE*.

A learned CC sets that track's parameter whatever channel it comes on, and no longer does what the standard map gave it (a CC learned for one parameter is taken off the one it had before; up to 16 CCs). The CCs with a meaning of their own are never learned: bank select (0, 32), mod wheel (1), data entry (6, 38), expression (11), sustain (64), 96–101 and the channel modes (120–127). What is learned is a setting of the FM-1, kept through power-off and projects, and in the web editor's backups (restored with them, on this FM-1 or another); **HOME menu → SYSTEM → MIDI LEARN CLEAR**, OCT+, forgets it all.

---

## 22. USB audio

On USB the FM-1 is also an audio input named **Felucca**: 44.1 or 48 kHz (the computer, phone or tablet picks; 48 kHz since 2.5, for the phones and apps that only take it), 16-bit stereo, class compliant, no driver. In your DAW, in Audacity or in a sampler app on your phone (with a USB-C cable or adapter) choose that input and record: you get the master output, exactly what the headphones play (after DUST, DUCK and FILT; the click and count-in too if they are on). MIDI, the editor and the installer keep working on the same cable.

**HOME menu → AUDIO → USB AUDIO:**

- **MASTER** (default): the recording follows the MASTER knob, as the headphones do. Keep MASTER well up while you record.
- **FULL**: a fixed level, as with MASTER all the way up (kept from clipping by the limiter); MASTER then only sets the headphones. Best for a computer input with no level control of its own.

The first time, the computer sees the FM-1 as a slightly different device (MIDI + audio) and sets it up again. On macOS 13–15, keep **USB SERIAL** OFF (the default) or the audio input may not show (2.6; from 2.7 the FM-1's build has no serial console and no USB SERIAL: the flash is full, and it was a developer's tool).

<a id="usb-rate"></a>**HOME menu → AUDIO → USB RATE** (2.7), KNOB 3: the rates the FM-1 offers. It takes effect at the next start: turn the FM-1 off and on (the menu shows *RESTART* until then).

- **AUTO** (default): 44.1 and 48 kHz, the computer or phone picks, as in 2.5 and 2.6.
- **44.1 KHZ**: 44.1 kHz only, the samples exactly as the FM-1 makes them (as in 2.4). If a phone or tablet records the FM-1 distorted or noisy while a computer records it clean, try this first: the phone then converts the rate itself.
- **48 KHZ**: 48 kHz only, for a device or app that needs it.

The computer or phone sees each setting as a slightly different device and may set it up again.

---

## 23. Your own samples and drum kits

The FM-1 has **four sample slots**, **USR1–USR4**, of about **7.4 s** each (22 kHz mono). A slot holds either:

- **an instrument**, played by a synth track: engine **SAMPLE**, **SET = USR1 … USR4** (or the GRAIN engine's SRC), or
- **a drum kit**, played by the drum track: **KIT = USR1 … USR4**, or **USR3+4**, one big kit of about **15 s** over two slots.

A good setup: USR1 and USR2 as instruments, a big kit on USR3+4. You fill the slots from the web editor.

### An instrument (editor → Samples)

1. **Choose a slot.** Four tiles show what each slot holds and how full it is; the page warns when sending would replace something (or half of your USR3+4 kit).
2. **Make the sound**, one of two ways:
   - **From files (one note per file):** drop up to 16 WAV files (any rate, mono or stereo). Each plays at its own note, the keys between play the nearest one, pitched. The note is read from the file name (`KEYS_C4.wav`, C4 = 60) or set in the list; a keyboard picture shows which keys play which file; ▶ plays a file; the meter shows the time used. Each file has a **Gain** (0 to −24 dB, 2.6); **Match levels** sets them so every file is heard at the same level (the loud ones turned down: every file comes in at full scale), then adjust by ear. **Loop** (2.7), for each file: **ONE SHOT** (plays once to the end, as before), **LOOP** (the whole file again and again while the key is held; let go, it ends its round) or **LOOP+TAIL** (the start plays, then the part between **Loop start** and **Loop end** again and again while the key is held; let go, the rest of the file, the tail, plays to the end). A short sample becomes a sound you can hold: a pad, a string, a voice, an organ. Set the points on the file's wave (the loop is lit); when you let go of a slider the point moves to the nearest zero crossing, so the loop does not click. Hold ▶ to hear it as the FM-1 plays it, let go for the tail. A WAV that has its own loop (set in a sample editor: its *smpl* chunk) comes in as LOOP+TAIL with its points. It takes no extra memory. The SAMPLE engine's **LOOP** (EDIT 1, KNOB 4) still turns loops off for the whole set. The release (ENV) still ends the note: set REL long enough to hear the tail. On SLOOP 2.6 such a file loops until the release ends it.
   - **Chop a recording** (CHOP): open or drop a recording (WAV, MP3, AIFF…) and cut it into up to 16 chops, one per key.
3. **Send it and play it:** name it, **Send to USRn**, then **Play this slot on track 1, 2 or 3**: that track switches to SAMPLE with SET = USRn.

**CHOP** in detail:

| Control | Does |
| --- | --- |
| **TAP** (or the space bar) while it plays | a cut at that moment (*snap to the hit* puts it on the attack) |
| **Find hits** (with a sensitivity), **Grid**, **Equal parts** | cuts made for you |
| the box on a chop, **Keep all**, **Keep none**, **K** | keep or leave out chops (left-out chops go neither to the slot nor to the WAVs) |
| the length slider, or the handle at the bottom of the wave | shorten a chop; **Up to the next marker** undoes it |
| **Fit to slot** | shortens the longest chops just enough to fit |
| **Download WAVs** | the kept chops as WAV files |
| keys 1–9, 0 | listen to a chop |
| double-click / drag / ← → / Del | add / move / nudge / remove a marker |
| mouse wheel | zoom |

A recording of any length works: keep the chops you want, the slot takes about 7.4 s of them.

### A drum kit (editor → Drum kit)

1. **Your sounds.** Drop a WAV on any of the **16 pads** (KICK, KICK 2, SNARE, CLAP, HAT, OPEN HAT, PEDAL, RIM, SNARE 2, LOW TOM, HI TOM, CRASH, RIDE, SHAKER, CONGA, COWBELL), or **Choose files** to add several at once: they are sorted onto the pads by name (*kick*, *bd*, *snare*, *hh*, *open*, *crash*…; a second kick goes to KICK 2). Click a pad's wave to hear it. Each pad has **Pitch** (±12 semitones), **Gain** and **Length** (a cut ends with a short fade). **Match levels** (2.6, under the pads) sets every pad's Gain so the kit is heard even (as the ear hears it: the lows count less; the loud pads turned down to the quietest), then adjust by ear.
2. **Where it goes.** **USR3+4** (the big kit, about 15 s; the editor shares the sounds out over the two slots by itself), or a single slot **USR1–USR4** (about 7.4 s). Each choice shows what it holds now and what a send would replace. The meter shows the time used; too long? shorten the long sounds or press **Fit the slot**.
3. **Send it.** Name it, **Send**, then **Use … on the drum track** selects track 4 and sets its KIT (or turn PRESETS on the drum track: USR3+4 is the last kit).

**Save as ZIP** keeps the kit's 16 WAVs to share; **Open ZIP** (or dropping a .zip) loads one back.

On the FM-1 your kit works like any other: steps, levels, ratchets, fill conditions, the DRUMS grid, the keys, MIDI channel 10 (a note plays its lane's sound), the filter, the slicer, mute. The closed hat cuts the open one. A lane without a sound is silent, and so is an empty slot.

### Your own synthesised kits (SYN1–SYN4)

SLOOP 2.5: the editor's **Drum synth** page changes every value of a synthesised kit, sound by sound, and the FM-1 plays the change at once. Never stored, SYN1–SYN4 are copies of 808, 909, TRAP and TECHNO.

1. **Pick a kit**, SYN1 to SYN4, and give it a name (8 letters: the DRUMS screen shows it). **Start from** + **Copy** makes it a copy of any of the 32 kits.
2. **Pick a sound** (KICK, SNARE, CLAP, HAT, OPEN HAT, LOW TOM, HI TOM, CRASH, RIDE, SHAKER, CONGA, RIM, COWBELL, CLAVE, KICK 2, SNARE 2): it plays (also **Play**; **Play on change** plays it after every change).
3. **Change it.** Each value is a tile you drag up or down, or a bar; **double-click** puts back the value of the kit it started from.

| Group | Values |
| --- | --- |
| **Tone** | **WAVE** (OFF, SINE, TRI, SQUARE, FM, BELL) · **PITCH** (a note, its Hz) and **FINE** · **BEND** (semitones the pitch falls from at the hit) and **B.TIME** (how fast) · **HOLD** (at full level) and **DECAY** · **LEVEL** · **2ND** (a second partial: its ratio, the drum head's second mode) and **2ND LVL** · **CLICK** (the attack: the beater, the stick) |
| **Noise** | **NOISE** (OFF, WHITE, METAL: the 808's six squares, CYM: metal and white, CHIP: an 8-bit noise) · **CLAP** (three bursts, then the tail) · **LEVEL**, **HOLD**, **DECAY** · **HPF** (a high-pass on the noise) · **CHIP** (the chip noise's clock) |
| **Filter** | **FILTER** (OFF, LP, BP, HP) · **ON** (the noise, or ALL: the tone too) · **CUTOFF**, **RES** · **ENV** (octaves the cutoff starts above it, falling with the pitch) |
| **Out** | **DRIVE** · **LEVEL** (dB) |

**CRUSH** (bits off) and **S&H** (a lower sample rate) act on the whole kit.

4. **Store on the FM-1** keeps the four kits in flash (stop the song first); until then *not stored yet* shows (switched off before, the FM-1 may come back with the last stored kits). **Use on the drum track** sets KIT to it. **Save file / Open file**: a kit as a file, to share or to keep. A [backup](#backup-and-restore) holds the four kits too.

---

## 24. The web editor

Open it from the [installer page](https://isod89.github.io/sloop-fm1/), the [editor link](https://isod89.github.io/sloop-fm1/webapp/editor/), or **`OPEN-EDITOR.bat`**, in **Chrome or Edge** with the FM-1 on USB, and press **Connect** (allow MIDI). It follows the device live: turn a knob on the FM-1, the editor moves. It looks like the FM-1: the black screen, the four track colours, the FM-1's own pixel font. The language button switches English / Japanese.

**Offline (2.6).** *Download the editor* (under Connect, or on the installer page) saves the whole editor as one file for a computer, `sloop-editor-2.6.html`, also attached to each [release](https://github.com/isod89/sloop-fm1/releases). Double-click it: it opens in Chrome or Edge without internet (Chrome asks once to allow MIDI) and works as the online one. Its sound library (kept in the browser) is its own, apart from the online editor's: move sounds between them with Export / Import, or a backup.

**As an app, also on Android (2.6).** Open the [online editor](https://isod89.github.io/sloop-fm1/webapp/editor/) or the installer once with internet, then *Install the app* (under Connect, or on the installer page; or Chrome's menu → *Install app* / *Add to Home screen*). From then on the SLOOP app opens the editor without internet, and its link to the installer works offline too (with the firmware it installs). It stays the online site kept by the browser, so it uses the same sound library as the online editor, and it takes each new SLOOP version by itself the next time it opens online. On Android, use the app rather than the downloaded file: Chrome on Android does not allow MIDI in a downloaded file.

**Layout:** the side bar has the pages in three groups — **Track** (LIVE, Sound, Sequencer, Tracks, Song), **Sounds** (Library, Samples, Drum kit, Drum synth), **Device** (Projects, Settings) — and the connection. On top: the page title, the messages, and on the Track pages the **four tracks** (number, sound, engine): click one to select it. With the side bar focused, the arrow keys move between pages.

**Values:** each value is a tile you **drag up or down like a knob** (Shift: fine), or a bar: click it, then the wheel or the arrow keys (Shift: steps of 10); **double-click** resets it.

| Page | What you do there |
| --- | --- |
| **LIVE** (2.7) | the whole FM-1 under your hands for a show, from a computer, a tablet or a phone: see [LIVE](#live) |
| **Sound** | every parameter of the selected track, as on the device: a card per button (ENV, LFO, EDIT, VOICE, FX, SEL, ARP), its pages as rows of four coloured values. **Engine** and **Presets** pickers, **Init sound**, **Copy to library**, **Save to file / Load file** (a sound as a file). On an FM6 track, the **FM6** panel: the six operators, **Read from track**, **Send to track** (*Send while editing* to hear every change), **Store in bank** (B1–B27), **Import SysEx** (or drop a .syx), **Store cartridge in bank** (2.5: the ticked voices of a bank into B1… in one write, and as user presets if you choose), **Export SysEx**, **Export bank as SysEx**. **The drum track** (2.7): **DRUMS** — the kit (‹ ›), level, pan, filter, reverb, delay, the INSERT, and the 16 pads: tap one to hear it, its **tune** and **decay** as the DRUMS screen's SOUND page (**all pads as the kit** puts them back) |
| **Sequencer** | the pattern settings (LEN, DIV, SWG, GATE) and the steps as the device's tiles; the **step list** takes typed note names (*C4 E4 G4*). On the drum track: a **grid** of the 16 sounds × the steps, with the **kit**: choose a **level** (GHOST, SOFT, NORM, HARD) and a **roll** (x1–x4), then click a cell — a hit; click again (same level and roll) — cleared; **Shift+click** — one level louder. Click a step for its **detail**: **Nudge**, **Parameter locks** (+ lock, pick the parameter, set or delete), **Fill** (normal, fill only, no fill). **Reload**, **Clear sequence**. **Length**: − / + or type 1–64. **Grid / Piano roll** and **Export MIDI / Import MIDI**: see below |
| **Tracks** | the four channel strips: level, pan, mute, select; SOLO and REC shown as on the device |
| **Song** | the song order of the [SONG screen](#14-song-mode-sections-chains-songs): a row per section (A–D), its **bars** (1–64) and how many **times** in a row; **▲ ▼** move a row, **✕** removes it, **Add**; **At the end**: stop or start again. **Read from the FM-1**, **Send to the FM-1** (16 entries in all; stop the song first). A section whose slot is empty is marked: save it on the FM-1 first (SAVE + key 5–8) |
| **Library** | your sound library, kept in the browser: **Save current sound**, search by name or tag, filter by engine, sort, **Audition**, **To device slot**, **Rename**, **Tags**, **Duplicate**, **Delete**, **Export** / **Import** / **Export library**; copy factory presets in. Next to it the **32 user presets on the device**: **Load**, **Store current sound**, **To library**, **Erase**, **Export bank**. Drag sounds between the library and the slots |
| **Samples** | your instruments in USR1–USR4, in three steps (see [above](#an-instrument-editor--samples)), with CHOP |
| **Drum kit** | your own drum kits, in three steps (see [above](#a-drum-kit-editor--drum-kit)) |
| **Drum synth** | your own synthesised kits SYN1–SYN4: every value of every sound (see [above](#your-own-synthesised-kits-syn1syn4)) |
| **Song & sections** | the sections A–D (**Load** into the working loop, **Save** it into one), **Songs** (2.7: **Export this song**, the loop, A–D and their order as one *.sloopsong* file; **Load a song file**; the LIVE page's playlist plays them) and **Backup** (everything on the FM-1) |
| **Settings** | GLOBAL (BPM, SWING, CLICK, TUNE), **MASTER** (DUST, DUCK, FILT, ROLL), DRUMS (CH, LVL, REV, DLY), and the system information (firmware, sync, slots) |
| **Theme** (2.7) | your own screen colours: see [Your theme](#your-theme) |

<a id="live"></a>**LIVE (2.7).** The editor's **LIVE** page plays the FM-1 in front of an audience: everything a show needs on one screen,
with the mouse, a finger on a tablet or a phone, or the computer's keys. What its buttons do goes through the FM-1's own code
(the same as GLO + key, SAVE + key, FX + key, REC, PLAY), so the FM-1 and the page always agree, and you can play on both at once.

- **Transport**: **PLAY / STOP**, **REC** (the selected track, as the REC button), **undo / redo**, **BPM** (drag the knob, or **TAP**),
  **SWING**, where the loop is (bar.beat) and, in song mode, which part and bar of the song.
- **Song**: **LOOP / SONG**, **SONG REC**, **FILL** while held and **FILL BAR** (the next bar is a fill), as GLO + key 9 and 10.
- **Sections A–D**: tap one: it plays from the next bar (stopped: at once); the one playing is lit, the one coming blinks, an empty one is
  dim. **Save the loop into A B C D** stores the loop (over a used section it asks first).
- **Mixer**, one strip per track: the **sound** (any preset; the drums: the kit) with **‹ ›** for the next one, the **level** fader with
  its meter, the pattern's progress, **filter** (left: low-pass, right: high-pass), **pan**, **reverb**, **delay**, **chorus** and **drive**
  (the drums: filter, pan, reverb, delay), **mute**, **solo**, and **play**: the keys below play that track (it is selected on the FM-1 too,
  so REC records it).
- **Master FX**: the master **filter**, **dust** and **duck** (FX + KNOB 1 to 3 on the FM-1).
- **Punch-in effects**: the 27 effects in four groups (loops & time, filter & EQ, colour, movement). Hold a pad: the effect plays while
  you hold it, as FX + key. **Latch**: a tap turns it on, another tap (or another effect) off.
- **Keys**: two octaves for tracks 1–3 (octave, velocity, the track's **arp** mode and **hold**), the 16 drum pads for the drum track
  (GHOST, SOFT, NORM, HARD). They are real MIDI notes on the track's channel (GLO > MIDI), so they sound at once.
- **Playlist**: your songs, one after the other. Export each song first (**Song & sections → Export this song**: the loop, the sections
  A–D and their order in one *.sloopsong* file), **add songs** here, and tap one (or **next song**, key **N**): the FM-1 stops, the
  song loads in a few seconds (its A–D, loop and order replace the FM-1's) and, with **play when loaded**, starts. The list stays in
  the browser; **save the list** keeps it as one file to take to a gig (**add songs** opens it again).
- **Full screen**: the page alone, the side bar hidden. **All off** lets go of every note and effect.
- **Help on the page**: every card says in one line what it is for, its **?** explains how to use it, and the mouse over any control says
  what it does. **First live in 5 steps** shows at the top until you press *got it* (its **?** brings it back). **view: essentials** keeps
  the transport, sections, mixer, keys and effects; **view: everything** adds the scenes, PERFORM, the playlist and the stage tools.
- **Sections or scenes?** The sections A–D change **what plays** (the notes: intro, verse, chorus…). The scenes change **how it sounds**
  (the knobs: filters, levels, effects), never the notes. Example: scene 1 "intro" with the filters closed and the drums low, scene 2
  "drop" with everything open: play your loop, tap 1, then 2 at the drop — or let scene 2 come over 8 bars for a build-up.
- **Scenes** (8): a scene keeps how everything sounds now — each track's level, mute, pan, filter, sends and drive, its sound's knobs
  (the engine's 8 values, FLT PIT SHP, the envelope, glide, the LFO), its INSERT and arp, and the master bus (filter, dust, duck, the
  delay, the reverb, the chorus, TEXTURE, swing). **+ sounds** keeps the presets and the kit too, **+ tempo** the BPM. **Store** then a
  scene pad keeps it (double-click: its name); a tap recalls it **at once** or **morphs** there over 1 to 16 bars. The **crossfader**
  mixes two scenes (A and B) as you move it. Kept in the browser, and in the setlist file.
- **Perform**: the track the keys play, in four tabs: **sound** (the engine's values, the macros, the envelope and glide, the LFO),
  **arp & key** (MODE, RATE, OCT, GATE, SWG, ORD, HOLD; ROOT, SCALE, CHORD, TRANSPOSE, for **every synth track** at once if you want),
  **insert & bus** (the track's INSERT and filter; the delay, reverb, chorus, TEXTURE and the metronome) and **steps** (the 16 steps of a
  page, a drum lane a row: a tap turns a step on or off, a new synth step plays the last note you played; a right-click its fill
  condition; LEN, DIV, SWG, GATE).
- **Sounds**: **☆** on a strip makes its sound a favourite (at the top of every list, in this browser); your **USER** presets are in the lists too.
- **MIDI learn**: press **MIDI learn**, tap a control of the page (a fader, a knob, a scene, an effect pad, PLAY…), then move a knob or
  press a pad on your controller (any USB MIDI controller plugged into the computer). Learned controls work as soon as the page shows;
  a right-click in learn mode forgets one, **forget all** every one.
- **The FM-1's live tools** (2.7): **chain** (under the sections: tap 2+ letters, **play chain**: each section for its bars, looped, as
  SAVE + several keys), **roll / erase** for the keys and pads (roll repeats while held at the **roll rate**, as ARP + key; erase removes
  that note or sound while held, as EDIT + key), the pattern's **shift ◀ ▶, ½ / ×2 length, transpose** (PERFORM → steps, as EDIT +
  KNOB 1–3), each **pad's tune and decay** (PERFORM → sound on the drums), **rev throw / echo throw** while held and the **volume**
  (MASTER FX; turning the FM-1's MASTER knob, or **knob**, gives it back).
- **Stage tools**: the **clock** and the **set's time** (from the first PLAY; tap it to start again), **lock** (the page ignores taps,
  so nothing changes by accident; the keys of the computer and the controller still work; hold it to unlock) and **record set**: the
  FM-1's USB audio (the browser asks for the microphone once) recorded as a WAV file until you stop. In the playlist, **✎** gives a
  song its **tempo** (set when it loads) and a **cue** (shown while it plays, the next one under it).

**The computer's keys** (not in a text field): **Space** play / stop · **1–4** sections A–D · **5–8** mute tracks 1–4 (**Shift**: solo) ·
**9** fill while held · **0** fill bar · **B** tap · **C** REC · **N** next song · **Z / X** octave · **← →** the track the keys play · **A W S E D F T G Y H U J
K O L P** play (the drums: the 16 pads) · **Shift** + those keys: the first 16 effects while held · **F1–F8** scenes (**Shift**: store) · **Esc** all off.

The mixer and the keys also work with an FM-1 on SLOOP 2.6; the transport, sections, fills and effects need 2.7. If the keys stay silent,
the page says why: GLO > SYSTEM > IN = CLOCK (no notes), or a track with no channel while OTHER is OFF.

<a id="your-theme"></a>**Your theme (2.7).** The editor's **Theme** page makes the FM-1's screen your own, with or without the FM-1 connected:

- **Screen**: five colours, faintest first (darkest on a dark background, lightest on a light one): **Lines**, **Dim** (empty steps, units), **Labels**, **Secondary** text, **Values**. **Start from** takes a built-in palette (the other way round on a light background); **Five steps from one colour** makes the five from the colour you pick.
- **Background**: black by default, or any colour, white too (**Back to black** returns to it). **Light ↔ dark** turns the theme over: a white background, the five steps the other way, a black accent (and back). The greys of the pages (tiles, empty steps, labels) follow the background.
- **Accent**: what you touch, the menu's cursor, the play head: white by default; on a light background a dark one.
- **Track colours**: T1, T2, T3 and the drums (the knobs' values, the step tiles, the meters).
- **Name**: up to 8 characters; the FM-1 shows it in MENU → COLOR.
- The preview draws a SLOOP page with your colours, exactly as the screen will show them.
- **Send to the FM-1** (stop the song first): the FM-1 keeps it and shows it at once. It then comes after MONO in **MENU → SCREEN → COLOR**; turn KNOB 1 to go back to a built-in palette and to your theme again. **Read from the FM-1** brings it back into the page; **Remove from the FM-1** deletes it. Send again after any change: next to the buttons the page says whether what you see is what the FM-1 has. **Values** and the **Accent** must stand out from the background so the menu stays readable: the page marks the one too close, Send says why, **Make it readable** moves just those two until they do, and the FM-1 refuses one too close. A theme made with a 2.7 beta still loads, with a white accent.
- **Share**: the line in the Share box (*SLOOP-THEME-1:…*) is the whole theme. Copy it into a comment or a message; anyone pastes it into the field below and presses **Use**.

A backup made with the editor keeps your theme. Back on SLOOP 2.6 the FM-1 shows the built-in palette you used last, and forgets the theme when it next saves its settings.

### Piano roll and MIDI files (Sequencer)

**Piano roll** (a synth track): the steps left to right, the notes bottom to top (**Octave up / down** scroll). Choose a **note length** (1/1 to 1/16 of a bar, in steps of the track's DIV), then:

- click an empty spot: a note of that length. On the first step of a chord: one more note of it (4 at most, one length for all, as on the device);
- click a note: it goes (the last note of a chord leaves rests);
- drag the right end of a note: shorter or longer (ties), up to the next chord.

Every change goes to the FM-1 at once, and the grid shows the same steps.

**Export MIDI** saves the selected track's pattern as a MIDI file (`sloop-track1.mid`…): the notes with their lengths, levels and accents, at the song's tempo; the drum track as GM drum notes on channel 10.

**Import MIDI** reads a MIDI file into the selected track, at its DIV. First a preview: the file's track to take, how many notes and steps, and what changes on the way: notes off the grid moved to the nearest step, more than 4 notes on a step (the loudest kept), a note cut short by the next one, notes past step 64. **Import** replaces the track's pattern (and, ticked, sets its length); **Cancel** leaves it as it was. On the drum track, the GM notes go to the lanes (36 kick, 38 snare, 42 hat…) and the velocity to the levels, ghost to hard.

The editor writes presets, the FM6 bank and backups to flash only while the song is stopped, as the FM-1 does (it asks you to stop first). Try it without hardware: add `?mock=1` to the editor's address. The protocol is documented in [web/EDITOR_PROTOCOL.md](web/EDITOR_PROTOCOL.md).

---

## 25. Backup, rescue, going back, troubleshooting

### Backup and restore

Editor → **Projects** → **Backup**:

- **Save a backup** writes everything on the FM-1 to one file (`SLOOP-backup-DATE.json`): the music you are working on, the sections A–D, the 32 user presets, the FM6 bank, the SYN drum kits, the samples **USR1–USR4** (your drum kits with them) and the settings (colours, calibration, song order, lights, SYNC, MIDI OUT).
- **Restore from a file** puts it all back: what is on the FM-1 is replaced. Stop the song (PLAY) first. A damaged file is refused before anything is written, and each object is written as a save writes it (a cut-off restore never leaves half an object).

A 2.4 backup restores into 2.4 (or later) only. **Save a backup before every update and before going back.**

### Rescue and special modes

| Situation | Do |
| --- | --- |
| SLOOP does not start | hold **OCT−** alone while switching on (*SLOOP USB RESCUE*), then install again from the browser |
| An install stopped half-way | the FM-1 waits in update mode: press **INSTALL** again |
| Buttons or knobs do the wrong thing | hold **OCT− + OCT+** while switching on, or HOME menu → SYSTEM → HARDWARE CALIBRATION |
| A developer needs UBOOT | hold **OCT− + OCT+** for 5 s while stopped (*UPDATE MODE IN 3…*; let go to cancel) |

### Going back to the official firmware

On the installer page, open **Return to the official firmware (V15)**: save a backup with the editor first, download FM-1 V15 from m-vave.com, select its `FM-1.fwsc` (only that exact file is accepted) and install it. M-VAVE's own updater (M-UPGRADE) works too; close every other app that uses MIDI first. To come back, install SLOOP again and restore your backup.

### Troubleshooting

| Problem | Fix |
| --- | --- |
| The installer or editor does not find the FM-1 | Chrome or Edge, a data cable, no hub, allow MIDI; close every other app or tab that uses MIDI, then reload |
| The black keys are silent on a synth track | it plays chords or the scale: SEL → CHORD OFF and KEYS OFF (with a chord on, the black keys change the chord: CHORD+) |
| Recorded notes move | SLOOP quantises to the track's DIV: REC screen [QUANT off](#quant-off) (2.7) keeps your timing; or use 1/32, or nudge the step (SEQ + step + KNOB 4) |
| Notes fade out on a dense part | the processor is at its limit; SLOOP fades one voice at a time (never the bass or lead) rather than glitching. Fewer held notes or a lighter engine help |
| Nothing from the MIDI IN jack | try the other adapter type (A / B); check the channel (GLO → MIDI: CH1–CH4; MIDI 2: OTHER) and that GLO → MIDI 2 → IN is NOTES |
| The USB audio input does not show | replug the FM-1; on a Mac (2.6) keep USB SERIAL OFF and restart the FM-1; in Audacity: Transport → Rescan Audio Devices |
| The USB recording is quiet | HOME menu → USB AUDIO = FULL, or MASTER up |
| The USB recording is distorted or noisy on a phone (clean on a computer) | HOME menu → AUDIO → [USB RATE](#usb-rate) = 44.1 KHZ, then turn the FM-1 off and on |
| An FM6 sound changed after loading a project | store the edited patch in the bank (editor → FM6 → Store in bank) and set PTCH to it |
| *STOP THE SONG FIRST* / *STOP FIRST* | that change (new project, song edit, flash write) waits until the transport is stopped |
| *EMPTY SECTION: REC* | the section you asked for has nothing in it: save a loop into it first (SAVE + key 5–8) |
| Something else | [open an issue](../../issues): what you did, what you expected, what happened, and the version (HOME menu → SYSTEM → ABOUT) |

---

## 26. Cheat sheet

### Buttons alone

| Do | Result |
| --- | --- |
| PLAY | start / stop |
| REC | record / arm / close a free take |
| hold REC ~2 s | clear the selected track |
| tap HOME | TRACKS · on TRACKS: visualiser |
| hold HOME | menu |
| tap SAVE | on TRACKS: SONG screen · else SAVE pages |
| tap ENV, LFO, FX, SEL, EDIT, ARP, SEQ, GLO | their pages (again: next page) |
| tap EDIT / SEQ on TRACKS, drum track | DRUMS screen (again: grid → kit → groove) |
| OCT− / OCT+ | octave (both: 0) · drums: ghost / hard while held |
| ALGORITHM | track |
| PRESETS | sound · drum kit |
| SELECT | tempo · pages · visualiser style · menu section |

### Layers

| Hold + | Keys | KNOB 1 | KNOB 2 | KNOB 3 | KNOB 4 | Other |
| --- | --- | --- | --- | --- | --- | --- |
| **LFO** | playable keyboard | vibrato Hz | depth | waveform | beat sync | + HOME: lock |
| **ENV** | playable keyboard | tremolo Hz | depth | waveform | beat sync | + HOME: lock |
| **FX** | punch-in effects: 16 white keys, 11 black keys | master FILTER | DUST | DUCK | track FILTER | + HOME: lock |
| **EDIT** | erase sound / note · black key: drum MUTE mode (white keys mute / unmute) | SHIFT | LENGTH ×2 / ½ | TRANSPOSE | OCTAVE | OCT− undo · OCT+ redo |
| **ARP** | note repeat | RATE | — | — | — | OCT− / OCT+: ghost / hard (drums) |
| **SEQ** | steps · black keys 1–4: page | SOUND / NOTE | DIV | SWING | LENGTH | OCT− / OCT+: page |
| **SEQ + step** | more steps | drums: which sound · synths: NOTE | drums: LEVEL · synths: VEL | RATCHET | NUDGE | PRESETS lock · ALGORITHM lock parameter · OCT+ fill condition · OCT− clear nudge, locks, condition |
| **SEL** | key of the song | CHORD | SCALE | KEYS | TRANSPOSE | OCT+: toggle scale lights |
| **GLO** | 1–4 mute · 5–8 solo · 9 fill · 10 fill bar · 16 tap tempo · F#3 MIDI LEARN | level 1 | level 2 | level 3 | level 4 | SELECT: tempo |
| **SAVE** | 1–4 play A–D (several: chain) · 5–8 save A–D · 13 loop / song · 14 SONG REC · 16 song screen | — | — | — | — | — |
| **any layer + HOME** | lock it open | | | | | any other button unlocks |

### Chord mode (CHORD on)

| Hold | + white key |
| --- | --- |
| nothing | the scale's chord on that key (C4 = I) |
| F# | major ↔ minor |
| G# | + 7th |
| A# | sus4 |
| C# | + 9th |
| D# | inversion |

### Power-on and special

| Do | Result |
| --- | --- |
| OCT− held at power-on | USB rescue |
| OCT− + OCT+ held at power-on | hardware calibration |
| OCT− + OCT+ held 5 s, stopped | update mode (UBOOT) |
| EDIT + OCT− / OCT+ | undo / redo |
| GO values (LOAD, SAVE, ERASE, NEW, CLRSQ, INIT) | one click arms (*AGAIN: …*), a second within ~1.5 s acts |
| SAVE + key 5–8 on a used section | press again within 3 s |
| SONG screen: REC on a used section · OCT+ | press again within 3 s |

### Where is…?

| Looking for | It is |
| --- | --- |
| tempo | SELECT on TRACKS · GLO + key 16 (tap) · GLO → GLOBAL → BPM |
| swing | TRACKS KNOB 1 (global) · SEQ + KNOB 3 (track) · GLO → GLOBAL |
| metronome | GLO → GLOBAL → CLICK |
| a track's level / pan | TRACKS KNOB 2 / KNOB 4 · GLO + KNOB 1–4 |
| the drum kit | PRESETS on the drum track · DRUMS kit page KNOB 1 |
| the drum level / reverb | DRUMS kit page KNOB 2 / 3 · GLO → DRUMS |
| the drum delay (2.5) | GLO → DRUMS → DLY (KNOB 4) |
| a track's filter | FX + KNOB 4 · FX → FILTER |
| delay time | FX → DLY → TIME |
| chords | SEL + KNOB 1 · SEL page |
| STRUM, VLEAD | SEL 2 |
| your projects | SAVE + keys 1–4 (play) / 5–8 (save) · SAVE → PROJECT (LOAD, SAVE, ERASE) |
| your sounds | SAVE → USER · PRESETS (after the factory sounds) |
| MIDI out / clock / clock only | GLO → SYSTEM: MIDI · SYNC · IN |
| each track's MIDI channel | GLO → MIDI (CH1 CH2 CH3 CH4) |
| MIDI in / out, the other channels, the bend | GLO → MIDI 2 (IN OTHER OUT BEND) |
| a vinyl / tape / rain bed under the mix | GLO → TEXTURE (2.6) |
| MIDI LEARN | GLO + F#3 · forget: hold HOME → SYSTEM → MIDI LEARN CLEAR |
| a track's effect (INSERT) | FX → INSERT, INSERT 2 |
| tempo-synced envelope / LFO | ENV DEST → ESYNC · LFO → LFO 2 → SYNC |
| ANALOG band-pass / high-pass | EDIT → FILTER → TYPE |
| a note's length in steps | SEQ → STEP page, KNOB 3 on the note (LEN) |
| drum mutes | EDIT + a black key, then the sounds' keys |
| lights, colours, USB audio level | hold HOME (menu) |
| the version | hold HOME → SYSTEM → ABOUT |

---

*SLOOP is based on Felucca by Leo Kuroshita (@kurogedelic), Hügelton Instruments. Full credits and licences: [SLOOP.md](SLOOP.md#rescue-going-back-credits) and [LICENSING.md](LICENSING.md). M-VAVE and FM-1 are trademarks of their owners; SLOOP is not affiliated with them.*

### Recorded chords driving the arpeggiator

On a synth track, turn ARP on, then **ARP 2 → ORD → SNOTE** (pitch order) or
**SPLAY** (live press / stored note order). Recorded chord steps now feed the arp;
the sequencer does not also play the chord directly. TIE sustains its input, REST
clears it, and live keys still contribute. NOTE/PLAY restore the previous live-only
arp plus direct sequence behavior; ARP OFF plays the recorded steps directly.
STOP clears the sequence and live latch. The arp keeps its own rate/gate/probability.
Factory sound recall may reset ORD. This does not create follower tracks or infer roots.

On **ARP**, KNOB 1 chooses MODE, KNOB 2 RATE, KNOB 3 octave range, and KNOB 4 GATE.
For a C–E–G–A chord held by ties, UP plays C–E–G–A, DN plays A–G–E–C,
and UPDN plays C–E–G–A–G–E. Changes take effect on the next arp tick.
Turning ARP OFF and back on while a chord is sustained transfers that chord
between direct sequence playback and the arp without waiting for the next NOTE.

### Native starter libraries

On a synth track, tap **SEQ** through STEP → PATTERN → RECORD → SEQUENCES → SONG.
The SELECT knob reaches the same five pages in either direction; SONG is the arrangement
screen. SEQUENCES offers 24 four-bar progressions, including classic turns,
sevenths, gospel, disco, garage, Latin rhythms, house offbeats and funk. Knobs select
starter, root, scale and CHORD/BASS/ARP NOTES output. A CHR track starts this browser
in Major without changing its actual scale. On any starter page, turn **PRESET**
to select **RHY** independently: ORIGINAL or 17 named attack rhythms. ORIGINAL
retains each starter rhythm; explicit choices control attacks in all three modes.
Its rhythm page offers rotation, offset,
syncopation and Feel. The next SELECT knob position opens **seq pitch**: knob 1 **OCTAVE**,
2 **ROOT**, 3 **SCALE**, 4 **VLEAD**. Octave controls the generated pattern independently
of the physical playing octave. VLEAD chooses nearby chord inversions, also used by
ARP NOTES; BASS remains on roots. Preview and Apply use the same pitches.

On drums, tap **SEQ** or **EDIT** through GRID → KIT → SOUND → GROOVE → RHYTHM.
The bank contains 24 beats, including Amen, funk, broken house, Afro clave,
jungle and odd-length patterns. RHYTHM knobs choose target lane, whole-step shift,
syncopation and Feel; Feel nudges the entire drum step.

In either browser, **OCT-** auditions without replacing the pattern. **OCT+** applies
while stopped; replacing existing work requires another OCT+ press or a 700 ms hold.
Changing the selection cancels confirmation. Apply produces ordinary editable steps;
the existing undo restores the replaced steps and their timing/lock metadata.

### Persistent keyboard scale guide (Merthsoft.8)

Hold physical **SEL** (labelled SCL by SLOOP), then press **OCT+** to toggle
**SCALE LIGHTS**. Release both and keep playing: scale notes glow dimly and pressed
notes remain bright. From Merthsoft.9, the guide and held SEL display follow the
selected synth's ROOT/SCALE shown on its SEL page; ordinary keyboard backlighting
is suppressed while the guide is active. Held SEL key/scale changes still apply to
all synths; tapped-page changes remain per-track. CHR
includes all twelve pitch classes. It marks physical pitch classes, without changing
transposition, octave, chord mapping or quantization. Drum/grid and held/locked
performance layers retain their own LED meanings. Toggle again to turn it off.
This gesture changes neither octave nor latch, even while notes are held. The setting
is saved with device preferences; if changed during playback, stop before powering off
so the deferred settings write can finish.

### Merthsoft.7 STEP shortcuts and companion additions

On synth STEP, hold either OCT button and turn **knob 2** for whole-octave pitch changes, or **knob 3** to move an event left/right. Moving forward into its own tie run shifts the onset later while keeping its end; other moves shift the complete onset/tie chain. Hold OCT+ or OCT− with knob 1 to paint ties/rests. Movements require stopped transport; octave editing preserves intervals and metadata.

Turn the **SELECT knob** within the musical browser to visit sequences, seq rhythm and seq pitch without interrupting audition. The physical SEL button opens chord/scale controls. The Android Sequence workspace can browse the firmware musical bank, audition through FM1, and confirm replacement with the same settings; firmware 2.5 Merthsoft.7 is required. Android Perform offers opt-in external MIDI device/port input for literal keys or bounded scale-root chords. Arpeggio output on FM1 carries live/sequenced attack velocity.

### Independent starter rhythm

In SEQUENCES, turn **PRESET** to select **RHY** independently of the progression.
ORIGINAL retains the starter rhythm; 17 named choices include BAR, QUARTERS,
EIGHTHS, OFFBEATS, FUNK, GARAGE and LATIN. Explicit rhythms also control ARP NOTES
attack spacing. Preview and Apply use the same selection.

**FEEL** shifts every event slightly earlier (negative) or later (positive), in
1/64 of a step. It does not alternate note timing like swing. Rotate/offset move
whole steps; syncopate moves eligible attacks onto the preceding offbeat.

### Layer a starter; preserve track effects

In SEQUENCES, ALGORITHM selects REPLACE or ADD (shown beside RHY). ADD preserves
the current pattern length, division, swing, locks and fill; duplicate pitches count
once. More than four different pitches at any step shows 4 NOTE LIMIT and changes
nothing. EDIT + OCT− / OCT+ undoes/redoes a successful apply. Ties are combined;
when either layer changes, the combined chord attacks again. Listen previews only
the starter. Android's existing musical-starter Apply remains a replacement.

Instrument browsing preserves the selected track's POLY/MONO/LEGATO/UNISON mode,
glide amount/mode, note priority/allocation, drive, chorus, delay and reverb
sends, insert, slicer and filter. On a normal FX-family page, press both octave
buttons together to reset those effects to their parameter defaults; individual
OCT buttons on these pages do not transpose. This reset does not change the sound
engine, sequence, key, or global effects. INIT SOUND resets the whole sound.



### Swing eighth-note grooves

Hold **HOME → TIMING → SWING GRID**, then choose **1/8** with knob 1. Return to
TRACKS and turn **SWING8** to set the amount. This shuffles eighth-note hats such
as HALF TIME even when the pattern uses a 1/16 grid. **STEP** preserves the old
swing behavior, following each track's DIV. The choice survives power-off; both
global and local swing amounts follow it. Native previews and the GROOVE
visualizer follow the timing too. Live ARP retains its separate ARP 2 SWG control.

### Write a custom progression

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

![Native custom 5–1–2–4 progression](assets/screens/merthsoft/guide/custom-progression.png)

### Independent note sustains

Use **POLY** for guitar-like overlaps. In STEP, select the starting note/chord and
hold **OCT+**. **Knob 4** chooses ALL or one source pitch; **knob 1** extends that
selection through later attacks. C can keep ringing through E and G without
restarting its envelope. Release OCT+ and turn **knob 3 (LEN)** to shorten the
selection, or tap **EDIT** to remove it and its following continuations. Moving
the cursor normally clears the voice selection. A full step or a deliberate new
attack of the same pitch refuses the edit without overwriting notes. Undo/redo
works for the complete gesture.

Single-event pitch/octave edits and moves are guarded while a pattern contains
independent ties. Use **held EDIT + knob 3/4** for whole-pattern semitone/octave
transposition, or delete/re-enter and extend a voice. Whole-step ties still use
the existing controls. Live recording and phone/web length editing have not yet
been upgraded to create independent durations; use the native STEP controls.

While extending, NOTE and LEN describe the source: **SRC** for all source voices,
**SEL** for one selected pitch. LEN stays anchored after shortening beyond the
cursor. Moving STEP normally clears that selection. The roll shows every tied
voice across 16-step banks and keeps notes visible while turning knobs.


### MerthSloop track auto-wah and auto-pan

Hold ENV+FX for wah, LFO+FX for pan. Keys remain playable with scale lights intact.
Wah knobs are rate/sensitivity, depth, low-pass base center and source (free LFO,
beat division or ENVELOPE). Pan knobs are rate, depth, waveform and beat sync.
Hold the combination and tap HOME to lock it; HOME, STOP/panic or track changes
clear it. These live controls retain saved patch settings. See the
[wah/pan controls](docs/firmware/AUTO-WAH-DESIGN.md) and
[MerthSloop version policy](docs/BRANDING.md).

### Master LOFI characters

Open **FX → LOFI** and choose VINYL for record colour on the full mix. TYPE defaults to **OFF**. **WOW** adds slow,
stereo-linked slow wow plus flutter; **DARK** darkens the highs; **DAMAGE** adds
pops and a little hiss. Start around WOW 15%, TONE 25%, CRACKLE 10% and adjust to
taste. These controls remain active until changed; the VINYL punch-in applies a temporary preset and restores them on release. All three at zero
bypasses the effect exactly. Settings are saved per project and song section.
Noise fades out at STOP; wow and tone still affect live playing. The separate
**GLO → TEXTURE → VINYL** is an additive background bed, so keep it off unless
you want extra record noise.

### Spring reflection timing

On **FX → REVERB**, TYPE selects STEREO, SPRING, ROOM, PLATE or HALL. With SPRING selected,
knob 2 **SIZE** sets the nominal decay target, knob 3 **DAMP** darkens the tail,
and knob 4 **TIME** sets the spring round-trip spacing from 20 to 80 ms. Early
pickups occur around one-half and three-quarters of that time, followed by the
dispersive tail. This is reflection spacing, not a separate pre-delay. Feedback
is compensated for TIME so SIZE's nominal decay target stays independent;
actual tails also depend on damping, dispersion and the sound. FREEZE holds its
current length until released. TIME is saved with the section/project and is
inactive for STEREO reverb.

The VINYL punch-in replaces the former MONO key (D#5). It uses a temporary
WOW/TONE/CRACKLE preset without modifying the persistent FX → LOFI controls.
Releasing it fades back to those controls. DOUBLE TIME replaces the former
octave-shift key (D#4): the song runs at twice its stored BPM, preserving pitch,
and releasing returns to normal or a still-held HALF gesture. Both rate gestures
are inactive under external SYNC.

### Native NOTES and action sheets

SEQ cycles STEP → NOTES → NOTE ACTIONS → PATTERN → RECORD → SEQUENCES → SONG. On STEP/NOTES, PRESETS selects 8/16/32/64 visible steps: turn left to zoom in, right to zoom out. The footer shows PRESETS: ZOOM / OCT: TOOLS. NOTES knobs are EVENT / PITCH / LEN / FORCE: EVENT visits individual attacks within chords and empty steps; PITCH moves the selected sustained chain, LEN changes its duration, FORCE changes its GHOST/SOFT/NORM/HARD level. Step accent remains shared. EDIT taps erase only the selected chain; EDIT+OCT−/OCT+ undo/redo. Existing OCT-held brushes remain STEP tools on both roll pages.

NOTE ACTIONS uses knob 1 to select a row and knob 2 to adjust its value/destination. Shared slide/nudge/fill/accent rows say STEP and update with knob 2. Move/copy destinations are staged; OCT+ applies once and OCT− returns to NOTES. Move/copy reject full/colliding destinations, pattern-end overflow and source step locks. Moving within the note's own sustain is allowed. Shared timing/fill/locks stay with their steps. These operations require stopped transport, preserve other voices and support undo.

Drums add HIT ACTIONS after RHYTHM, retaining the selected lane and step. Ratchet is per hit; nudge and fill are shared per step. Move/copy/delete preserve the other lanes; occupied same-lane destinations refuse the change. OCT− returns to GRID. Function layers retain priority; action-sheet physical keys are silent, and PLAY/REC leave the sheet normally.

LOFI TYPE starts at and defaults to **OFF**: it fades to dry without changing the three stored
amounts. Selecting a character restores those amounts; VINYL punch remains
momentary over OFF and returns to dry on release. OFF is saved with the project
and section. Its inactive controls show the retained WOW/DARK/DAMAGE amounts.

### Delay characters

Press FX, then turn SELECT to **DLY TYPE** (page 7). Knob 1 chooses **DIGI**,
**TAPE** or **CRUSH**. Knobs 2/3/4 follow the type:

| TYPE | Knob 2 | Knob 3 | Knob 4 |
| --- | --- | --- | --- |
| DIGI | TIME | FDBK | MIX |
| TAPE | DRIVE | TONE | MIX |
| CRUSH | BITS | RATE | MIX |

DIGI retains the original sound. TAPE DRIVE sets soft saturation; zero removes
it, and TONE darkens the feedback. CRUSH BITS selects 4–16 bits and RATE selects
about 2.8/5.5/11/22/44 kHz. Higher settings are cleaner; 16 bits and 44 kHz
leave the tap unchanged. The preceding **DLY** page keeps TIME/FDBK/COLR/MIX
for every type. All character settings are retained when switching types. These are shared global settings; each track's DLY send controls how
much of that instrument enters the delay. Dry sound is unchanged.

Try TIME 1/8, FDBK around 60–80%, MIX around 70%, and a clearly audible track
send. Switch TYPE while playing to hear the difference. Type changes fade
smoothly through the original delay. The type saves with the project/section;
older untagged saves use DIGI. TAPE currently adds saturation, not wow/flutter.

![Tape delay controls](assets/screens/merthsoft/neon/delay-tape.png)
