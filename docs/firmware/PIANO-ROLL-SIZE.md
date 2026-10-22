# Piano editor target size measurement

Measured on October 21, 2026 using a frozen private copy of the working tree. The complete new piano feature set costs **7,768 bytes (7.59 KiB) of target image** and **32 bytes of static transient RAM** in this controlled comparison. These are actual JieLi pi32v2 target builds, not host object estimates.

| Target result | Original STEP roll, new features removed | Complete new features | Difference |
| --- | ---: | ---: | ---: |
| Application image | 577,820 B | 585,588 B | **+7,768 B** |
| `.text` (code and read-only data) | 574,280 B | 582,048 B | +7,768 B |
| `.ram_text` | 2,956 B | 2,956 B | 0 |
| `.c1_text` | 60 B | 60 B | 0 |
| `.data` | 524 B | 524 B | 0 |
| `.bss` | 85,048 B | 85,080 B | **+32 B** |
| `.lowram` | 22,772 B | 22,772 B | 0 |
| `.pool` | 332,876 B | 332,876 B | 0 |
| `.noinit` | 15,464 B | 15,464 B | 0 |

The production application slot is 581,564 bytes (`0x8DFBC`). This baseline has 3,744 bytes free; the full-feature snapshot exceeds it by **4,024 bytes**. Both private linkers were enlarged equally to `0xADFBC` solely to obtain complete linked measurements. The full-feature measurement image is **not flashable** within the production slot. No package was generated.

The scope includes 8/16/32/64 STEP zoom, NOTES selection and pitch/length/force editing, NOTE ACTIONS and HIT ACTIONS UI, selected-chain pitch/move/copy helpers, drum hit helpers, rendering/cache changes, keyboard ownership and project-generation invalidation. The baseline retains the original STEP roll and native tie editing. Both variants retain MIDI updates, LOFI, expanded reverbs, drum FX fixes and all four accepted space savings. Existing STEP length-delta clamping remains in both variants. No features were disabled to improve the full-feature number.

## Target symbol breakdown

These are linked symbol size differences, including compiler inlining and layout effects. They are useful for locating cost, but are not independent feature toggle measurements.

| Symbols | Target image delta |
| --- | ---: |
| `actions_*` and `drum_hit_*` functions | +3,188 B |
| `note_chain_action`, `note_chain_pitch`, `note_chain_place` | +1,056 B |
| `draw_columns` | +688 B |
| `step_edit` | +592 B |
| `piano_selection_*` | +452 B |
| `graph_roll` | +410 B |
| All remaining named `.text` symbols, net | +1,040 B |
| Unnamed/read-only contents and alignment, net | +342 B |
| **Total image increase** | **+7,768 B** |

Largest individual additions: `note_chain_action` 998 B, `actions_draw` 926 B, `actions_apply` 572 B, `actions_edit` 454 B and `actions_input` 332 B. Existing `draw_columns` grows from 2,598 to 3,286 B; `step_edit` from 2,784 to 3,376 B; `graph_roll` from 462 to 872 B. Named `.text` differences sum to 7,426 B; the remaining 342 B is included in the linked section/image delta above. Small unrelated-function size changes reflect compiler layout/inlining effects in the single compilation unit, not source changes to those functions.

The 32-byte `.bss` increase measures static editor state and alignment. For example, `ui` grows from 436 to 444 bytes. This is not a measured stack high-water delta; helper and renderer automatic variables use the existing stack, whose runtime peak was not measured here. There is no project/track storage or pool increase.

## Reproduction and evidence

Private root: `build/note-chain-actions/measurement/`. The input snapshot manifest identity, before the identical measurement-only linker enlargement, is:

```
d89db7a629b59e55f7078b4b9b5aaa6c9d073ba324b8972858ae70a3c5ffade1
```

`snapshot-sha256.json` records the full copied inputs. `full-build-input-sha256.json` and `baseline-build-input-sha256.json` record final build inputs. `piano-only.diff` records the exact comparison, modifying only params, project, seq, ui, ui_draw, ui_input and ui_studio. Unreferenced helper files remain present in the baseline but are not included. Original STEP functions were restored from read-only `git show HEAD` results; unrelated working-tree changes were retained.

Each variant has `run.py`, `result.json`, `sections.txt`, `symbols.txt`, and its private `build/felucca.elf`, `.bin` and `.dis`. `symbol-delta.tsv` and `.json` contain the complete symbol comparison. The scripts call only `tools/build.py::build_app()`, with its normal `-Oz -ffunction-sections -fno-builtin -Wall -Wno-unused-function` flags, boot tables, the default product identity, and `FELUCCA_SLICE=0`. The installed JieLi toolchain is `jieli-linux-toolchains-20250324.1`. Existing generated headers were copied; generators, version updates, package creation and shared target outputs were not invoked.

Later working-tree integration changes can change absolute size and the delta. These numbers identify this frozen comparison, not a claim about a subsequently edited build or package 36.

## Integrated production build

After applying the separate 4,448-byte descriptor/startup-table savings, the
complete current source builds with the unmodified production linker to 581,140
bytes, leaving 424 bytes. Main static RAM is 89,064/98,304. The enlarged-linker
measurements above remain controlled historical comparisons, not release images.
