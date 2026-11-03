# Native C++ YouTube client: implementation and verification

Reference source: [yt-dlp b77c9304e1c4220cca450c583491904daf8455af](https://github.com/yt-dlp/yt-dlp/tree/b77c9304e1c4220cca450c583491904daf8455af).
The pinned reference is linked above so behavior can be compared against upstream.

The new executable is an independent C++17 implementation. It does not invoke
Python, yt-dlp or a browser. The inherited custom downloader CLI is not compiled
into this executable. The separate Desktop `ytcui-dl` project is preserved.

## Implemented paths

| Area | Native implementation |
| --- | --- |
| HTTP and platform | libcurl HTTPS, connection reuse, proxy and IP-family options; Windows/POSIX process launching without a shell; UTF-8 Windows argument handling |
| Player extraction | Watch-page bootstrap, current context/visitor data, player URL/STS discovery, fourteen translated client configurations, anonymous/authenticated default client sequences and format merging |
| JavaScript challenges | C++ batching, signature/n URL rewriting and manifest n rewriting; pinned EJS 0.8.0 in Node with permissions enabled |
| Persistent cache | Player code, preprocessed EJS and resolved challenges; atomic writes and expiry checks |
| Accounts and tokens | Explicit Netscape cookies, expiry filtering, three SID authorization schemes, delegated/user session headers, client-specific cookie policies and separate player/GVS/subtitle tokens; C++ token-provider callback |
| Formats | MIME/codec parsing, languages and DRC tracks, strict numeric/string filters, IDs, audio/video/muxed choices, combinations and fallbacks |
| HLS and DASH | Master variants/audio groups, relative URLs, MPD representations and BaseURLs, SegmentList, SegmentTemplate, timeline expansion and selected-representation playback manifests |
| Downloads | Parallel validated byte ranges, committed-chunk resume, sequential fallback, native fragment fetching, simultaneous audio/video transfers, finalization and ffmpeg mux/transcode |
| Live and clips | Manifest transport selection, bounded recording, live-start option, clip section discovery and requested time sections |
| Comments | Entity and legacy formats, top/newest ordering, parent/reply/subthread handling, hearts/pinned/verified/uploader flags, estimated timestamps, limits, deduplication and continuation-loop checks |
| Collections | Ordered playlist pagination, search, channels/tabs, Shorts view models, community posts, feeds, mixes and notifications |
| Chat | Public live/replay API pagination, action preservation and configurable message/page limits |
| Subtitles | Manual/ASR tracks, translation languages, VTT/TTML/srv1/srv2/srv3/json3 URLs and saving; separate subtitle/audio language selection |
| CLI and integration | Native Windows launchers/package, Linux build, multiple inputs, batch files, archives, common output templates, JSON/sidecars, mpv argv output and CMake `ytwe::core` embedding target |

Primary modules are `we_http.h`, `we_platform.h`, `we_cache.h`, `we_clients.h`,
`yt_youtube.h`, `yt_js.h`, `yt_media.h`, `we_formats.h`, `we_manifests.h`,
`we_collections.h`, `we_subtitles.h`, `we_selector.h` and `we_download.h`.

## Verified on this machine

- The Windows application and its DLL package launch without WSL or the MSYS2
  toolchain on PATH. Native mpv is bundled. Node and ffmpeg are available on PATH.
- Windows CTest: four suites covering the scanner, YouTube extraction, portable
  core and authentication scoping. Linux CTest: the same suites plus the original
  client and selection suites, six total.
- A local HTTP fixture verified byte-identical parallel-range and fragmented
  downloads, correct fallback when Range is ignored, rejection of malformed
  Content-Range responses, and resume without refetching committed chunks.
- Live native search and channel extraction returned ordered results. Playlist
  extraction exhausted all eleven entries of the Computerphile test playlist.
- Live top-comment extraction returned eight comments, including five replies,
  with correct parent IDs; newest comments were also fetched.
- Big Buck Bunny resolved 59 merged client formats, including 4K60. A native 144p
  download passed ffprobe verification. Separate downloaded tracks were muxed and
  verified to contain both audio and video.
- The bundled Windows mpv decoded a three-second segment using separate resolved
  video/audio URLs. A filename containing spaces, Unicode, ampersand and dollar
  characters was downloaded successfully without shell interpretation.
- A public live-channel alias resolved to its current video, and the public live
  chat API returned five actions. A bounded live recording passed ffprobe with
  both tracks and a duration of 3.018 seconds. Live transport selection prefers
  manifests, since raw segmented live URLs are not ordinary media files.
- Subtitle metadata exposed five manual languages and 156 automatic/translation
  languages on the tested music video. A native English WebVTT download was
  verified. WEB translation context is combined with a usable client caption
  URL; the upstream `xosf` cleanup and subtitle token context are preserved.

Account-only feeds/notifications/private/member content were not exercised with
an actual user account. Synthetic tests verify cookie and SID headers, delegated
account fields, selected-client player tokens and omission of credentials from
unsupported clients and third-party/CDN hosts. No browser profiles were accessed.

## Measured performance

A local benchmark extracted the same video's itag 140 URL with the Samsung TV
client. Each implementation received one warm-up, then three timed runs.

| Implementation | Median |
| --- | ---: |
| Native C++ | 2.07 seconds |
| Cloned yt-dlp | 10.38 seconds |

That is about 5.0x for this warm-cache workload on this machine. The implementations
perform different amounts of metadata work, and network timing varied; it is not
a general claim for every video or download. The native implementation caches
preprocessed JavaScript between runs.

## Differences from the entire yt-dlp application

This is not a drop-in reproduction of every yt-dlp option, plugin or postprocessor.
It does not load Python plugins. Advanced output-template traversal/conversions,
the complete format-expression grammar, every metadata fallback and all specialized
live/DVR/post-live edge cases are not identical to upstream. The implemented
commands reject unsupported options rather than accepting them silently.

BotGuard token minting is supplied by the embedding application/provider, not by
this client. JavaScript remains necessary to execute changing YouTube player
code; EJS is deliberately retained as JavaScript, as it is in yt-dlp. ffmpeg/mpv
remain runtime media tools. The first-party C++ logic does not delegate YouTube
extraction to either Python or yt-dlp.

EJS provenance/hashes are in `vendor/ejs/PROVENANCE.md`; pugixml source provenance
and its MIT license are in `vendor/pugixml`. The original project MIT license and
Git history remain intact. The maintained native project is published at
[MilkmanAbi/ytcui-we](https://github.com/MilkmanAbi/ytcui-we).
Automated build, runtime and package checks are described in the README.
