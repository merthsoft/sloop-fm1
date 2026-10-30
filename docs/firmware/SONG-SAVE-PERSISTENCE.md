# SONG SAVE and the power-on working loop

The SONG screen previously wrote settings/arrangement order only, while reporting SONG SAVED. It did not commit the working project, so edits such as master filter LP63 → OFF still depended on background autosave. Two explicit saves did not change that behavior.

The stopped SONG save now captures the working project and uses the existing autosave_commit path, which also saves arrangement/settings/snapshots. It reports SAVE ERROR if either the working project or settings/snapshots fail. It does not overwrite the separately stored A–D sections. Playback/transport guards remain. Project format and autosave idle/quiet policies are unchanged.

The NOR-model regression reproduces the original failure: seed LP63, clear to zero, invoke arrangement_save twice, reset RAM and call autosave_resume. It fails before this change and passes afterward. Coverage also checks nonzero filter/mixer/swing values, arrangement order, failed settings writes with retry, untouched section storage, and background zero autosave after its idle/quiet conditions are met. The background case passes, and the user subsequently confirmed that waiting at zero did preserve it on hardware. The fix is specifically to the explicit SONG SAVE path.

Build 2.8.0.103: 583,912 / 584,336 B, 424 B free. Main/low/pool RAM remain 97,724 / 29,288 / 335,424 B. Evidence: ignored build/save-103.
