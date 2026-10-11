# Latest firmware — 2.5 Merthsoft.19

- Native SEQUENCES uses PRESET to select ORIGINAL or 17 independent attack rhythms, shared by Preview and Apply and retained across tracks.
- Explicit rhythms override ARP NOTES' implicit eighth-note pulse. Chord BAR uses sustained ties; existing rhythm shaping and VLEAD remain independent.
- Existing phone requests use ORIGINAL, without inheriting/overwriting the native rhythm choice. Android selection is not implemented in this release.

Target image **579,312 bytes**; static RAM **96,244/98,304** (2,060 free); pool **333,948/344,064**. HAL and call-free RAM checks pass.
Package **610,066 bytes**, identity **FM-1_900**. SHA-256: `ffa3d63855f45cca7c8d0ad8b933cd0c17c76d1b6376ebe882c47216cbb342e7`.
Installer: http://127.0.0.1:8805/webapp/installer/

All rhythm/progression/mode attack-mask checks, tie/gate checks, transformed hit-count preservation, native selector/retention and phone compatibility tests pass. Broad UI regression passes. Physical audition remains user testing.

The intermittent save-loss report remains unresolved; the user reports the latest flash retained the song successfully. No saving fix is claimed.
