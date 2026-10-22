# MIDI expression and bank selection integration

Implemented in the shared checkout without replacing upstream files. Settings and
FUN6 bytes are unchanged. Existing modulation, arp ownership, browsing preferences,
raw samples, clock and reverb changes remain in place.

## Behavior

CC11 sets runtime track expression using upstream's squared GM gain. CC121 restores
full expression and retains MerthSloop modulation reset. CC120/123 retain their
existing modulation behavior. Track LEVEL and drum mix settings are unchanged.
`proj_apply` resets all expression, covering full load, section adoption, autosave
resume, load undo/redo and arrangement restoration.

CC0 latches a bank for each channel independently on USB and TRS. Program Change
captures that bank alongside the queued program; subsequent CC0 cannot retarget
an already queued request. IN=CH filters CC0 with the other messages; IN=CLOCK
ignores it. CC32 is unchanged/ignored. CC121 does not clear bank selection.
`up_pc_index` resolves the existing base factory list followed by occupied user
slots, excluding favourites. Bank 8 resolves physical user slots, including gaps;
empty slots, out-of-range programs and unavailable linear banks return -1.
Drum Program Changes retain kit numbering and ignore bank selection.

## Parent hooks (now integrated by parent; not edited here)

In `firmware/src/ui.c`, add alongside the other upreset forward declarations:

```c
static int32_t up_pc_index(uint32_t bank, uint32_t program, uint32_t factories);
```

In `pc_poll`, capture `bank` with `r` under IRQ exclusion before clearing the
request, so a concurrently arriving Program Change cannot pair a new bank with
an old program:

```c
uint32_t r, bank, n;
fm1_irq_off();
r = pc_req[i]; bank = pc_bank[i];
if (r) pc_req[i] = 0;
fm1_irq_on();
```

Keep the existing drum branch. Replace the synth `else if (preset_go_to...)` with:

```c
} else {
    int32_t index = up_pc_index(bank, n, NBANK);
    if (index < 0 || preset_go_to(&trk[i], (uint32_t)index))
        continue;
    n = (uint32_t)index;
}
```

Use `preset_go_to` for bank 8 as well: it preserves the existing track effects and
playing preferences around `up_load_to`. Do not copy upstream's inverted
`if (!up_load_to(...)) continue` condition or import preset-owned arp changes.

In `firmware/src/drums.c`, inside `drums_mix`, replace just the level expression:

```c
int32_t lvl = expr_gain(song.g[G_DRLVL] * 200, TRK_DRUM), send = /* existing remainder */
```

This applies expression to both sampled and synthesized drums before pan and
sends using existing level smoothing. The independent metronome remains unchanged.

## Verification

Private host outputs only: `build/midi-expression-bank`. Compile with assertions
active (`-UNDEBUG`), existing `build/gen`, no target or asset generation:

```sh
zig cc -UNDEBUG -O2 -w -Ibuild/gen -Ifirmware/src -Ifirmware/hal \
  tests/midi_expression_bank_test.c -lm \
  -o build/midi-expression-bank/midi_expression_bank_test
build/midi-expression-bank/midi_expression_bank_test
```

The regression uses actual engine rendering, MIDI event queues, project codec and
scene/restore paths. It checks squared synth attenuation and silence, expression
and saved-level isolation, CC121/modulation coexistence, source/channel bank
isolation, queued bank snapshots, selected-channel and CLOCK filtering, factory
boundaries, compact user mapping, physical bank-8 slots and invalid inputs.
Parent must apply the above hooks before claiming end-to-end drum audio or
Program Change loading coverage; bank indexing alone does not prove loading.

Confirmed private results: new MIDI regression PASS; existing `seq2_test` PASS
(including upstream clock averaging regressions). `git diff --check` passes.
After applying the drums hook, run with `MIDI_REQUIRE_DRUM_HOOK=1` to require
actual drum audio attenuation/silence across every sampled/synthesized kit.
The default test leaves that pending hook assertion disabled explicitly.
Existing `modulation_test` also passes, including live wheel, CC121, filter response and routing cleanup.


## Independent integration review

The parent has integrated UI and drum hooks. The new drum cache sentinel is
`drum_factory_set`; the private MIDI fixture now resets that variable.

- `MIDI_REQUIRE_DRUM_HOOK=1 build/midi-expression-bank/midi_expression_bank_test`
  PASS: sampled factory, factory synth and editable synth kits have squared
  expression attenuation and zero output at expression zero. Empty user sample
  slots (USR1..USR4/USR3+4) correctly produce silence at every expression setting.
- `build/midi-expression-bank/midi_bank_ui_test` PASS: actual production
  `pc_poll`/`preset_go_to`, factory bank boundary mapping, invalid bank/program
  rejection, bank-independent drum kit choice and saved mix/effects/appended arp
  preference preservation. The shared framebuffer harness doubles user preset
  loading, so this does not claim actual user-record adoption coverage.
- A transient duplicate G_LTYPE enum during concurrent editing was gone on the
  successful builds; no production edit was made by this review.

**Remaining load-order defect:** `pc_req`/`pc_bank` survive `proj_apply`. A MIDI
Program Change can be consumed in the audio callback, then a project/scene can be
adopted before `pc_poll` runs in the main loop. That stale request then changes
the newly adopted sound. The MIDI test prints a REVIEW line reproducing this
case; its other assertions pass. Recommended parent patch in the existing
per-track adoption loop beside `ex_off[k] = 0`:

```c
pc_req[k] = 0;
pc_bank[k] = 0;
```

The adoption loop already requires IRQ exclusion (or runs inside the ISR).
Keep the separate `ch_bank` latches intact: these are the external controller's
persistent bank state, not pending sound changes. Re-run the probe after this
patch; its REVIEW line should disappear. A pending request also survives
`arrangement_apply`/song restoration today because both call `proj_apply`.

All review builds used existing generated headers and private output paths.
No production sources, shared harness, target artifacts or assets were edited.

## Parent integration completion

The parent applied both hooks and clears queued pc_req/pc_bank during project/section adoption while retaining channel CC0 latches. The review probe is now an assertion. `midi_bank_ui_test.c` now includes production upreset.c and loads real memory-backed XIP user records through pc_poll/preset_go_to/up_load_to, including bank 8 and the compact linear list; the NOR programming primitive remains outside this loading test. Synth/drum expression audio, all kits, factory/user loads, invalid inputs and preference preservation pass. Both regressions are part of the main test runner.
