# Latest firmware — 2.5 Merthsoft.17

- Held LFO/ENV and synth EDIT show key-signature guides, including F# in E minor and Eb in C minor, with physical-key hints. Playing and erase mapping stay unchanged.
- Hold EDIT and turn knob 4 to shift the entire synth pattern by octaves. Knob 3 retains semitone shifts. Uniform bounds preserve chord intervals.
- EDIT shows SHIFT in steps, total TRANSPOSE in semitones and the octave-knob amount for the current/last gesture. Undo shows zero; redo restores it.
- Retained sequence-browser choices and shared visualizer memory from the previous builds remain integrated.

Target image **578,208 bytes**; static RAM **96,244/98,304** (2,060 free); pool **333,948/344,064**. HAL and call-free 925-instruction RAM code checks pass.

Package **610,066 bytes**, identity **FM-1_900**. SHA-256:
`2f8e1d51046a669618194feacf950f76b73f072cdc4a7fcf45d54ef6a0faaf66`.

Local installer: http://127.0.0.1:8803/webapp/installer/

Focused EDIT, modulation and broad 20,000-frame UI regressions pass. Physical testing is not claimed; existing historical LOFI golden and ISR-budget limitations remain. See [editing controls](../../docs/firmware/SEQUENCER-EDITING.md), [scale guides](../../docs/firmware/SCALE-MIDI-GRIDS.md), [verification](VERIFICATION.md), and [previous release](archive/releases/RELEASE-2.5-Merthsoft.16.md).
