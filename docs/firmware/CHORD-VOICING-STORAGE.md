# Chord voicing persistence

Status: the parent has implemented this carrier in project capture/apply. All valid settings, full/section adoption, invalid tags, metadata coexistence and neutral upstream fixture checks pass; real flash section/autosave cold-load checks also pass. Hardware startup/restore is not claimed. No wire-format or parameter-ID changes are needed.

## Recommended policy

Inversion and spread belong to each synth track, alongside its harmony settings. Store them in runtime `track_t` fields outside `p[]`, and preserve them when changing factory/user sounds. Include them in full projects, autosave, load undo and song sections. New projects and projects without the extension use inversion 0 / spread 0.

User preset records and editor parameter frames remain unchanged. A user preset does **not** capture or replace these track settings. This matches the existing preservation of ROOT, SCALE, CHORD and voice leading in `ui.c:param_kept` / `upreset.c:up_load_to`; make that policy explicit in the user guide. Loading a preset must not incidentally zero the new fields through a whole-track reset.

## Available carrier: G_NEWPRJ

`core.h` defines `G_NEWPRJ` as global ID 31. `project_t` stores 32 globals, starting at byte 8; its signed 16-bit word is therefore byte offset **70**. `params.c` defines it as `PE("NEW", N_GO, 0)`, where `N_GO` has the legal values 0 and 1. `project.c:proj_capture` currently copies that word without assigning metadata to it. Repository searches show no other persistence use for this word.

Use its high bit as the presence marker and the remaining 15 bits as three five-bit synth-track codes:

```text
word = 0x8000 | code[0] | code[1]<<5 | code[2]<<10
code = spread*7 + inversion_index
inversion_index: 0, +1, +2, +3, -1, -2, -3 => 0,1,2,3,4,5,6
```

Each valid code is 0..20. Decode `index = code % 7`, `spread = code / 7`, then `inversion = index <= 3 ? index : 3-index`. Codes 21..31 are invalid and must produce neutral settings for that track, never unchecked arithmetic or out-of-range fields. Reject absent marker values as neutral regardless of payload. The drum track has no entry.

When all three codes are zero, write zero instead of the marker. This preserves the complete canonical upstream default project, including its checksum. Do not place the tag in the live `song.g[G_NEWPRJ]`: it is a serialization detail, read from the raw saved word and written only into the captured record. The live NEW action stays zero after load. Write the carrier explicitly even when neutral, rather than relying on the live action state.

Add compile-time checks for `G_NEWPRJ == 31`, `NPART == 3`, `PROJ_NG == 32`, `sizeof(project_t) == 3816` and `offsetof(project_t,g[G_NEWPRJ]) == 70` (or equivalent standard-C offset expression). Existing P IDs, `P_COUNT`, project structs, magic and sizes stay unchanged.

### Old-reader behavior

Checked against `git show main:firmware/src/project.c`: full-project `proj_apply` copies each permitted global through its descriptor clamp. A tagged word is negative as `int16_t`; NEW clamps to 0 in its 0..1 range. The project loader directly assigns the clamped value and does not dispatch a NEW action. The action itself lives in `ui_input.c`'s explicit knob-handler switch, which resets the action value before calling `project_new()`.

For song-section adoption, upstream applies only the permitted mix globals and ignores NEW. A later upstream capture copies live NEW (normally zero), so upstream load-and-resave drops our voicing extension while retaining the ordinary project. This is graceful extension loss, **not** round-trip preservation through upstream. A raw object backup/export that does not apply/reserialize the project preserves the tag.

Our decoder must run for both `all=1` and `all=0`; placing it only inside the full-global adoption branch would silently lose voicing on section changes. Decode before/with track adoption; clear missing/invalid settings rather than inheriting the previously selected section.

## Occupied fields that must not be reused

| Existing carrier | Current consumer |
|---|---|
| `rsv[0]` | TEXTURE source/when/duck, 0x80 presence, 0x40 post-2.7 keyboard-HOLD migration |
| `rsv[1]` | TEXTURE rate |
| `g[G_DRCH]`, `g[G_INFO]` | TEXTURE level/tone and movement |
| drum `e[1..7]`, `g[G_SAVE]` | Per-lane tuning/decay and 0x5400 checksum marker |
| `g[G_LOAD]` | Reverb model/reflection timing (expanded 0x5800 and older tags) |
| `g[G_NAME]`, `g[G_ENGGO]` | Master LOFI amounts/type (0x8000 payload + 0x5C00 type tag and legacy forms) |
| `g[G_ENGSEL]` | Delay type, 0xED00 tag |
| `g[G_CLRSEQ]`, `g[G_INITSND]` | Delay drive (0xD400), crush bits/rate (0xD800) |

The apparent spare bits of TEXTURE, preset indices, note flags and parameter-lock records are not needed and should remain untouched. No assertion of safety is based merely on an unused-looking bit.

## Alternatives and tradeoffs

1. **Recommended: NEW action-word extension plus runtime track fields.** Zero record growth, stable IDs, neutral upstream behavior, fully persistent in our projects/sections. Cost is a tiny encoder/decoder and an explicitly documented user-preset exclusion.
2. **Append parameters / versioned project format.** Would require remapping the fixed FUN6 common/engine parameter layout, preset map-by-count behavior and editor clients. Not appropriate for two controls with an available carrier.
3. **User-preset tail metadata.** `up_rec_t.p[UP_PMAX]` has capacity 72 while current `P_COUNT` is 71, leaving one tail int16 outside the represented parameter count. It is not a complete solution: `up_parse` and `up_store` zero the record and fill only `P_COUNT` words; GET/PUT carries only that many, and `up_params` ignores the tail. It would disappear through existing editor serialization and would compete with future upstream parameters. Do not use without a separately versioned policy.
4. **Ephemeral UI fields.** Smallest implementation but loses settings on power cycle/section load. Rejected as the default because the user expects saved harmony settings to return.

## Validation gates

- Exhaust all 21 valid codes per track and distinct combinations across all three tracks; capture, checksum, import, full apply and section apply must round-trip exactly.
- Verify all-neutral capture is byte-identical to `tests/fixtures/upstream271.fun6` using `tests/upstream_project_test.c`, with unchanged P IDs and project size.
- Test marker missing (including legal old NEW=1), invalid codes 21..31, signed boundary words, one invalid track with valid neighbors, and loading a legacy/neutral project after non-neutral settings. No stale fields survive.
- Test the current upstream reader's clamp and capture behavior: negative NEW becomes zero, no NEW action runs, ordinary notes/key/mix remain intact, and upstream re-save drops only the extension.
- Round-trip voicing together with **all** occupied carriers above: TEXTURE, drum sound tuning, reverb, master LOFI, delay type/drive/crush. This catches accidental action-word collisions.
- Exercise save A..D, actual autosave/boot path, load undo and arranger section transitions; generic `proj_apply` coverage alone is insufficient for reset initialization or stale runtime fields.
- Preserve voicing across factory and user-preset changes, engine changes and explicit sound initialization according to the declared track-setting policy; NEW PROJECT resets it. User-preset record bytes and GET/PUT round trips remain unchanged.
- `proj_sum` already hashes every byte before `sum` using FNV-1a, so it covers byte 70. Assign the carrier **before** calculating the final checksum. A changed tag with the old checksum must be rejected by `proj_ok`/`proj_import`.
- Editor backup/restore transports raw objects with its existing CRC-32 validation and `project_restore`; phone song archives should retain the entire native payload and checksums. Test a non-neutral carrier through these paths rather than teaching them a new P ID.

Target image/RAM cost and hardware behavior remain parent integration measurements; this investigation makes no new build-size claim.
