# ytcui-we 0.2 — native C++ YouTube client

[![CI](https://github.com/MilkmanAbi/ytcui-we/actions/workflows/ci.yml/badge.svg)](https://github.com/MilkmanAbi/ytcui-we/actions/workflows/ci.yml)

Native Windows, Linux, macOS and BSD client. Its extraction, pagination, caching, format selection and download
control flow are C++17. It runs without Python, yt-dlp or a browser process.
YouTube JavaScript challenges use the pinned upstream EJS helper in Node; mpv
plays the resolved streams and ffmpeg performs muxing, transcoding and recording.

## Acknowledgments

**Thank you to [yt-dlp](https://github.com/yt-dlp/yt-dlp) and its contributors.**
This C++ implementation translates YouTube extraction behavior studied in its
[pinned source](https://github.com/yt-dlp/yt-dlp/tree/b77c9304e1c4220cca450c583491904daf8455af).
Its [EJS](https://github.com/yt-dlp/ejs) helper is bundled for player challenges.
This is an independent project and does not reproduce every yt-dlp feature.
See [THIRD_PARTY.md](THIRD_PARTY.md) for attribution and dependency licenses.
The original ytcui-dl history and MIT license are preserved.

## Downloads and CI

Download archives and SHA-256 checksums from [Releases](https://github.com/MilkmanAbi/ytcui-we/releases).
Windows archives include runtime DLLs; keep their contents together. POSIX
archives require the platform's libcurl, OpenSSL and zlib runtime packages.
Binaries are platform/architecture specific, not universally static executables.
Node >=24 is recommended for JavaScript challenges; install mpv for playback
and ffmpeg for merging or recording. These external programs are not bundled.

CI builds with GCC and Clang, including libc++, on Linux x64/ARM64, macOS
Intel/Apple Silicon, native Windows, Alpine, FreeBSD, OpenBSD and NetBSD.
It runs unit tests, byte-for-byte HTTP range/resume/fragment fixtures, CLI
error and PATH checks, Node/EJS process tests where Node >=24 is available,
ASan/UBSan, the Make wrapper, installation and unpacked archive smoke checks.
Tests run offline; passing CI cannot guarantee that YouTube's live endpoints
continue to accept every client. Release publishing is gated on the full matrix.
See [CI](.github/workflows/ci.yml) and [release](.github/workflows/release.yml).

## Run on Windows

For a source build, `build-native.ps1` places the executable and DLLs in `bin/`.
The launchers below use that directory. Release archives contain `bin/ytcui-we.exe`
and can also be launched directly. No WSL is needed. From PowerShell:

```powershell
.\ytcui-we.ps1 --help
.\ytcui-we.ps1 -p -q 720 aqz-KE-bpKQ
.\ytcui-we.ps1 -L aqz-KE-bpKQ
.\ytcui-we.ps1 -a -g aqz-KE-bpKQ
.\ytcui-we.ps1 comments aqz-KE-bpKQ --max-comments 20 --sort newest
.\ytcui-we.ps1 info aqz-KE-bpKQ
.\ytcui-we.ps1 search "Blender Foundation" --limit 5
.\ytcui-we.ps1 channel "https://www.youtube.com/@Blender/videos" --limit 5
```

`ytcui-we.cmd` is also available. The PowerShell launcher forwards arguments
directly to the executable. Quote URLs and paths containing shell characters.

## Media and files

A raw video URL downloads by default. An explicit `media` command prints metadata
unless an action is selected. `-g` prints separate video/audio URLs; `-p` supplies
both tracks and the source User-Agent to mpv.

```powershell
.\ytcui-we.ps1 media aqz-KE-bpKQ -g -f "bestvideo[height<=1080]+bestaudio/best"
.\ytcui-we.ps1 media aqz-KE-bpKQ -V -q 144 -d -o "video.mp4"
.\ytcui-we.ps1 -f "bestvideo[height<=720]+bestaudio" -o "%(title)s [%(id)s].%(ext)s" aqz-KE-bpKQ
.\ytcui-we.ps1 -x --audio-format mp3 aqz-KE-bpKQ
.\ytcui-we.ps1 media aqz-KE-bpKQ --mpv-args -q 720
.\ytcui-we.ps1 --skip-download --print id aqz-KE-bpKQ
```

The selector supports numeric/format IDs, best/worst audio/video/muxed choices,
`+` combinations, `/` fallbacks and strict numeric/string filters. Unsupported
syntax is rejected. Filters are not silently relaxed. Template fields include
id, title, channel, uploader, channel_id, ext, format_id, resolution, width,
height, duration, upload_date, playlist and playlist_index; padded integer
indices are supported. This is not yt-dlp's entire expression/template language.

Direct tracks use native parallel ranges with `.part`/`.ranges` resume state.
Responses are checked before committing each chunk. Servers that ignore ranges
fall back to a full sequential transfer. DASH fragments are fetched natively;
separate audio/video downloads run concurrently and are muxed by ffmpeg. Files
are finalized after successful completion. Existing final outputs are skipped
unless `--force-overwrites` is supplied.

```powershell
.\ytcui-we.ps1 -N 6 --download-archive archive.txt aqz-KE-bpKQ
.\ytcui-we.ps1 --batch-file urls.txt --ignore-errors
.\ytcui-we.ps1 record LIVE_URL --record-seconds 30 -o recording.mkv
.\ytcui-we.ps1 -d --download-sections "*10-20" -o excerpt.mkv aqz-KE-bpKQ
```

HLS/DASH manifests, live recording, `--live-from-start` and clip sections are
handled through the native extractor and ffmpeg/mpv. `--record-seconds` bounds
recording; an unbounded live recording continues until stopped or the stream ends.

## Comments, collections and subtitles

```powershell
.\ytcui-we.ps1 comments aqz-KE-bpKQ --sort top --max-comments 50 --max-replies-per-thread 5
.\ytcui-we.ps1 -J --write-comments --max-comments 20 aqz-KE-bpKQ
.\ytcui-we.ps1 playlist PLzH6n4zXuckpfMu_4Ff8E7Z1behQks5ba --limit 20
.\ytcui-we.ps1 channel "https://www.youtube.com/@Blender/shorts" --limit 10
.\ytcui-we.ps1 community "https://www.youtube.com/@Blender" --limit 10
.\ytcui-we.ps1 feed subscriptions --cookies cookies.txt --limit 10
.\ytcui-we.ps1 mix MIX_ID --limit 20
.\ytcui-we.ps1 notifications --cookies cookies.txt --limit 20
.\ytcui-we.ps1 live-chat VIDEO_ID --limit 100 --max-pages 10
.\ytcui-we.ps1 subtitles VIDEO_ID --lang en --sub-format vtt
.\ytcui-we.ps1 --skip-download --write-subs --sub-langs "en.*" VIDEO_ID
```

Comments and chat actions use JSON Lines. Collections use one JSON object with
ordered entries, page counts and an exhaustion flag. Modern view models and
legacy renderers are supported. Playlists preserve intentional repeated videos.
Default limits are 100 comments / 1000 collection entries / 1000 API pages;
comment replies default to five levels and 100 replies per thread. All are
configurable. Repeated pagination tokens stop extraction.

Manual captions, ASR and translation languages are exposed in VTT, TTML, srv1,
srv2, srv3 and json3. Use `--write-subs` / `--write-auto-subs` to save tracks;
`subtitles` prints URLs unless `-o FILE` is supplied. `--audio-lang` chooses an
audio language independently of subtitle language selection.

`--write-comments` includes comments in metadata and writes an info sidecar.
`--write-info-json` requests a sidecar without comments. Ordinary `info` prints
metadata without downloading the video.

## Accounts, tokens and JavaScript

`--cookies FILE` reads an explicitly supplied Netscape cookie file. The client
handles SID authorization, delegated account headers and client-specific cookie
policies. No browser profile is opened, decrypted or exported.

```powershell
.\ytcui-we.ps1 info VIDEO_ID --cookies cookies.txt
.\ytcui-we.ps1 media VIDEO_ID --client tv_samsung,visionos,web -L
.\ytcui-we.ps1 media VIDEO_ID --po-token "web.gvs+TOKEN" -g
```

Fourteen client configurations are translated from the pinned yt-dlp source.
The default chain follows its authenticated/anonymous choices and merges usable
formats. Player, GVS and subtitle token contexts remain distinct. Tokens can be
supplied explicitly; the C++ library also exposes a token-provider callback.
The project does not implement a BotGuard token-minting service.

Player code, preprocessed EJS and challenge results are cached persistently.
Use `--cache-dir DIR` or `--no-cache`. Node >=23.5 is required when a selected
client returns JS challenges; choose it with `--js-runtime PATH`. Helper assets
are pinned and bundled, not downloaded at runtime. Proxies and IPv4/IPv6 forcing
are supported.

## Build and use as a library

Windows uses the installed MSYS2 UCRT64 GCC, curl, OpenSSL and zlib development
packages plus CMake/Ninja:

```powershell
.\build-native.ps1
```

The script builds, tests and packages the executable and its DLL dependencies.
Use `-MpvDirectory` to include a locally installed mpv in your personal build. Node and ffmpeg must be available for the
operations that require them.

On Linux/macOS/BSD, install a C++17 toolchain, CMake, libcurl, OpenSSL and zlib:

```sh
make -j2
make test
# Or cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
#    cmake --build build -j2
#    ctest --test-dir build --output-on-failure
```

Include `include/ytwe.h` and link the `ytwe::core` CMake target when embedding the
client. The library exposes video resolution, comment/collection callbacks,
subtitles, manifests, selectors and downloads. Use one `Youtube` instance per
concurrent API worker. The public token-provider callback allows an application
to supply its own attestation integration.

See `PORT_STATUS.md` for source provenance, validation and known differences.
Python plugins and every advanced yt-dlp option/postprocessor are not reproduced.
The historical `README.upstream.md` describes the original merged repository,
including code paths that are not part of the new executable.
