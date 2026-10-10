# Latest firmware — 2.5 Merthsoft.15

Optimization release; all performance features, instruments, samples and the splash screen remain.

- Shared visualizer storage saves 1,536 static RAM bytes.
- Shared starter metadata handling and measured UI boundaries save 816 image bytes.
- Audio render hashes and tracked DSP/ISR costs match the preceding build.
- Regression checks cover shared histories, pitch mapping, undo and stress; test-oracle and stale-sanitizer runner issues were corrected.

Target image **577,056 bytes**; static RAM **96,212/98,304** (2,092 free); pool **333,948/344,064**. HAL and call-free 925-instruction RAM code checks pass.

Package **610,066 bytes**, identity **FM-1_900**. SHA-256:
17874cb08879526337158768707f8815a12a7cf5c750e9b4a3728069912a8f60.

Local installer: http://127.0.0.1:8801/webapp/installer/

Existing LOFI golden and historical ISR-budget failures reproduce on the untouched baseline. Physical device validation is not claimed. See [optimization evidence](../../docs/firmware/OPTIMIZATION.md), [verification](VERIFICATION.md), and [previous release](archive/releases/RELEASE-2.5-Merthsoft.14.md).
