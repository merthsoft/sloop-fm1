# Drum hit action helpers

`firmware/src/drum_hit_actions.c` is an include-style helper, included after
`drums.c` and `seq.c` have supplied the packed lane accessors and `trk_len`.
It allocates no persistent state or pattern buffer.

```c
static int drum_hit_place(track_t *t, uint32_t lane, uint32_t source,
                          uint32_t dest, int move, int write);
static int drum_hit_delete(track_t *t, uint32_t lane, uint32_t source, int write);
```

Pass `move=0` for COPY HIT and nonzero for MOVE HIT. Pass `write=0` for
read-only preflight, nonzero for commit. Each commit repeats all validation
before writing. The caller owns stopped-playback enforcement, concurrent edit
exclusion, undo capture and cursor changes. Preflight is not a reservation;
the caller must prevent intervening edits while capturing undo and committing.
These helpers never change transport or undo state.

| Result | Meaning |
| --- | --- |
| `DRUM_HIT_OK` (`1`) | Valid operation; commit writes the change |
| `DRUM_HIT_NOOP` (`0`) | MOVE of an existing hit to its own position; no write |
| `DRUM_HIT_BOUNDS` (`-1`) | Null track, invalid lane, or index outside physical/active pattern bounds |
| `DRUM_HIT_EMPTY` (`-2`) | Source lane has no hit |
| `DRUM_HIT_OCCUPIED` (`-3`) | Destination lane already has a hit, including COPY to self |

Capture undo only after result `DRUM_HIT_OK`; all other results leave every
byte unchanged. Source validation precedes destination validation. Empty-source
MOVE to self returns EMPTY. Active length follows the existing `trk_len`
semantics (nonpositive configured length means one step), with an additional
physical `NSTEP` index check. Unsigned indices cannot wrap into the pattern.
The caller supplies the intended drum track; the helpers do not infer track
identity from pointer or engine.

COPY/MOVE transfers only the selected lane's packed level and ratchet. Other
lanes can occupy the destination freely. MOVE and DELETE clear the source
lane's on bit, level and ratchet. All other lane expression bits, source and
destination nudge/fill/locks, parameters and runtime state stay byte-identical.
No shared metadata follows a hit. There is no loop wrapping.

`tests/drum_hit_actions_test.c` uses the real host simulator, track layout and
packed accessors. It checks all lanes and source slots for copy/move/delete,
all level/ratchet combinations, neighboring lanes and full-track byte equality,
preflight purity, self-operation policy, empty source, occupied destination,
null track, physical/active bounds, all pattern lengths and extreme unsigned
indices (including converted INT_MIN, INT_MAX and UINT_MAX).

Validation passed in WSL Ubuntu:

```sh
mkdir -p build/drum-hit-actions
build/host/optimization-cc -UNDEBUG -O2 -fwrapv -fno-strict-aliasing -w -Ibuild/gen -Ifirmware/src -Ifirmware/hal tests/drum_hit_actions_test.c -o build/drum-hit-actions/test -lm
build/drum-hit-actions/test
# drum hit actions: all assertions passed
```

No target build, flash fit measurement, UI wiring, undo integration or hardware
validation is claimed by this isolated helper regression.
