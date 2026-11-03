# Acknowledgments and third-party notices

ytcui-we builds on the work of **yt-dlp and its contributors**. The YouTube
extractor, client selection, pagination, authentication, format handling and
JavaScript challenge protocol were studied and translated into C++ using
[yt-dlp commit b77c9304e1c4220cca450c583491904daf8455af](https://github.com/yt-dlp/yt-dlp/tree/b77c9304e1c4220cca450c583491904daf8455af)
as the reference. yt-dlp's source is distributed under the Unlicense, with
separately licensed components; see its [license](https://github.com/yt-dlp/yt-dlp/blob/b77c9304e1c4220cca450c583491904daf8455af/LICENSE).
This project is independent and is not endorsed by yt-dlp.

- Original ytcui-dl: Abi, MIT; retained in LICENSE and upstream history.
- EJS 0.8.0: yt-dlp contributors, Unlicense. Its bundled Meriyah (ISC) and
  Astring (MIT) notices remain in yt.solver.lib.js. See vendor/ejs/LICENSE
  and vendor/ejs/PROVENANCE.md.
- pugixml: Arseny Kapoulkine, MIT; see vendor/pugixml/LICENSE.md and provenance.
- libcurl: curl contributors, curl license, https://curl.se/docs/copyright.html.
- OpenSSL 3: OpenSSL Project contributors, Apache-2.0,
  https://www.openssl.org/source/license.html.
- zlib: Jean-loup Gailly and Mark Adler, zlib license, https://zlib.net/zlib_license.html.
- Windows GCC runtime DLLs: GNU GPL with GCC Runtime Library Exception 3.1,
  https://www.gnu.org/licenses/gcc-exception-3.1.html.

Windows archives include dependency DLLs from MSYS2. The packaging script
copies the corresponding installed package license files into licenses/.
Other archives require their platform's curl, OpenSSL and zlib shared libraries.
Node, mpv and ffmpeg are optional external programs and are not distributed
in release archives. Install them separately for their respective operations.
