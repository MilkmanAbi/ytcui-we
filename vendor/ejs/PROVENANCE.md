# EJS dependency

- Upstream: https://github.com/yt-dlp/ejs/releases/tag/0.8.0
- Version: 0.8.0, as pinned by yt-dlp commit b77c9304e1c4220cca450c583491904daf8455af.
- `yt.solver.core.js`: copied from that yt-dlp checkout's vendored helper.
- `yt.solver.lib.js`: downloaded from the official 0.8.0 release.
- Both were verified with SHA3-512 against yt-dlp's vendored `_info.py`.

| File | SHA3-512 |
| --- | --- |
| yt.solver.core.js | c163a6f376db6ce3da47d516a28a8f2a0554ae95c58dc766f0a6e2b3894f2cef1ee07fa84beb442fa471aac4f300985added1657c7c94c4d1cfefe68920ab599 |
| yt.solver.lib.js | 1ee3753a8222fc855f5c39db30a9ccbb7967dbe1fb810e86dc9a89aa073a0907f294c720e9b65427d560a35aa1ce6af19ef854d9126a05ca00afe03f72047733 |

EJS is Unlicense. Bundled Meriyah (ISC) and Astring (MIT) license notices are
included at the top of the library file and must be retained.
