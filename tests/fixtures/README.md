# Upstream project oracle

`upstream271.fun6` was produced by `upstream271_project.c` compiled against
unmodified SLOOP commit `4ec1ac829fd2bdf2c571ea215fb3d62477d01220`, including
its own drum-table generator. It is 3,816 bytes, in upstream's native FUN6 format.
The source sets THUMB/WALK/CHORD arp IDs, overlapping four-note continuations,
nudges, a fill condition, a parameter lock and per-drum tune/decay.

Keep this independent of fork serializers. To regenerate deliberately, place that
upstream revision in a temporary directory, generate its headers, and compile the
producer with its `tests` and `firmware/src` include paths. Pass the destination
filename as its first argument. `tests/upstream_project_test.c` loads/applies/saves
the file byte-for-byte; Android's song-library tests use the same fixture.

`upstream28.fun6` is independently produced by `upstream28_project.c` against unmodified SLOOP 2.8 commit `b77bf55e3bf804665b8f5dffb5955bd8443311f0`. It adds standalone and mixed continuation/legato flags to the same canonical FUN6 layout. Both upstream fixtures must survive import, apply and capture byte-for-byte.
SHA-256: `f4de953aa682d6805a0721c40b5c0a902246157ec955eb66249f7b161fcd0f71`.
