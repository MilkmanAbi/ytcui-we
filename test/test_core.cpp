#include "we_collections.h"
#include "we_manifests.h"
#include "we_selector.h"
#include "we_subtitles.h"
#include "yt_select.h"
#include <iostream>
static int failures = 0;
static void check(bool value, const char *label) {
  if (!value) {
    ++failures;
    std::cerr << "FAIL: " << label << '\n';
  }
}
int main() {
  check(ytwe::join_url("https://example.org/a/master.m3u8?key=1",
                       "../video/stream.m3u8") ==
            "https://example.org/video/stream.m3u8",
        "relative manifest URL");
  auto attrs = ytwe::hls_attributes(
      "BANDWIDTH=1200,CODECS=\"avc1.64001f,mp4a.40.2\",RESOLUTION=1280x720");
  check(attrs["CODECS"] == "avc1.64001f,mp4a.40.2", "quoted HLS attributes");
  ytfast::VideoInfo hls;
  ytwe::parse_hls(
      "#EXTM3U\n#EXT-X-MEDIA:TYPE=AUDIO,GROUP-ID=\"aud\",LANGUAGE=\"en\",NAME="
      "\"English\",URI=\"audio.m3u8\"\n#EXT-X-STREAM-INF:BANDWIDTH=4000000,"
      "RESOLUTION=1920x1080,FRAME-RATE=60,CODECS=\"avc1.64002a,mp4a.40.2\","
      "AUDIO=\"aud\"\nvideo.m3u8\n",
      "https://example.org/a/master.m3u8", hls, "test", "ua");
  check(hls.formats.size() == 2 && hls.formats[0].language == "en" &&
            hls.formats[1].height == 1080 && hls.formats[1].fps == 60 &&
            !hls.formats[1].has_audio,
        "separate HLS audio/video metadata");
  ytfast::StreamFormat raw_live;
  raw_live.url = "https://example.org/segmented-live";
  hls.formats.push_back(raw_live);
  hls.is_live_now = true;
  ytwe::select_live_transports(hls);
  check(hls.formats.size() == 2,
        "live streams prefer manifest transports over non-file raw URLs");
  ytfast::VideoInfo dash;
  ytwe::parse_dash(
      R"(<MPD xmlns="urn:mpeg:dash:schema:mpd:2011" mediaPresentationDuration="PT10S"><Period><AdaptationSet mimeType="video/mp4" codecs="avc1.64001f"><SegmentTemplate timescale="1000" startNumber="3" initialization="init-$RepresentationID$.mp4" media="segment-$Number%05d$-$Time$.m4s"><SegmentTimeline><S t="0" d="2000" r="-1"/></SegmentTimeline></SegmentTemplate><Representation id="137" bandwidth="4000000" width="1920" height="1080" frameRate="60000/1001"/></AdaptationSet></Period></MPD>)",
      "https://example.org/mpd/main.mpd", dash, "test", "ua");
  check(dash.formats.size() == 1 && dash.formats[0].fragments.size() == 6 &&
            dash.formats[0].fps == 59,
        "DASH timeline expansion");
  check(dash.formats[0].fragments[1].url ==
                "https://example.org/mpd/segment-00003-0.m4s" &&
            dash.formats[0].fragments[5].url ==
                "https://example.org/mpd/segment-00007-8000.m4s",
        "DASH template substitution and time");
  ytfast::VideoInfo parsed;
  ytwe::parse_player(
      R"({"videoDetails":{"title":"Title","lengthSeconds":"10"},"streamingData":{"adaptiveFormats":[{"itag":140,"url":"https://example.org/a","mimeType":"audio/mp4; codecs=\"mp4a.40.2\"","audioTrack":{"id":"en.1","displayName":"English"},"contentLength":"128"},{"itag":137,"url":"https://example.org/v","mimeType":"video/mp4; codecs=\"avc1.640028\"","height":1080,"width":1920}]}})",
      "aqz-KE-bpKQ", parsed);
  auto selected =
      ytfast::Selector::select(parsed.formats, ytfast::Mode::AudioVideo, {});
  check(selected.ok() && selected.audio && selected.video &&
            selected.audio->language == "en",
        "portable parser/selection preserves language");
  auto strict = ytwe::select_format(
      parsed.formats,
      "bestvideo[height>=720][height<=1080]+bestaudio[language=en]/best",
      ytfast::Mode::AudioVideo, {});
  check(strict.video && strict.audio && strict.video->height == 1080,
        "strict range filters and merged selection");
  auto audio_only =
      ytwe::select_format(parsed.formats, "140", ytfast::Mode::AudioVideo, {});
  check(audio_only.mode == ytfast::Mode::AudioOnly && audio_only.audio,
        "numeric audio format changes selection mode");
  bool unavailable = false;
  try {
    ytwe::select_format(parsed.formats, "bestvideo[height>2160]",
                        ytfast::Mode::VideoOnly, {});
  } catch (const std::exception &) {
    unavailable = true;
  }
  check(unavailable, "strict filters never silently relax");
  parsed.title = "A: title";
  parsed.id = "abcdefghijk";
  check(ytwe::output_template("%(title)s-%(playlist_index)03d.%(ext)s", parsed,
                              parsed.formats[0], "m4a",
                              7) == "A_ title-007.m4a",
        "output templates sanitize values and pad indices");
  std::string long_name = "a";
  for (int i = 0; i < 50; ++i)
    long_name += u8"\U0001f600";
  check(ytwe::safe_filename(long_name).size() == 177,
        "long Unicode filenames truncate at UTF-8 boundaries");
  check(ytwe::safe_filename("CON.txt") == "_CON.txt",
        "Windows reserved filename protection");
  auto entry = ytwe::parse_entry(
      "shortsLockupViewModel",
      yj::parse(
          R"({"onTap":{"innertubeCommand":{"reelWatchEndpoint":{"videoId":"abcdefghijk"}}},"overlayMetadata":{"primaryText":{"content":"Short title"},"secondaryText":{"content":"1.2K views"}}})"));
  check(entry.id == "abcdefghijk" && entry.views == 1200 &&
            entry.title == "Short title",
        "shorts view model");
  auto subtitles = ytwe::subtitle_tracks(
      R"({"captions":{"playerCaptionsTracklistRenderer":{"captionTracks":[{"baseUrl":"https://example.org/c?fmt=srv3","languageCode":"en","name":{"simpleText":"English"},"isTranslatable":true}],"translationLanguages":[{"languageCode":"fr","languageName":{"simpleText":"French"}}]}}})");
  check(subtitles.size() == 12 && subtitles[0].ext == "vtt" &&
            subtitles[1].automatic && subtitles[1].translated,
        "subtitle formats/translations");
  check(ytwe::language_matches("en-US", "en.*,-fr") &&
            !ytwe::language_matches("fr", "all,-fr"),
        "subtitle language patterns");
  check(ytwe::parse_time_text("2 days ago (edited)", 1000000) == 827200 &&
            ytwe::parse_time_text("2024-01-01") == 1704067200,
        "relative and UTC timestamps");
  check(ytwe::timestamp_seconds("1:03.5") == 63.5,
        "section timestamps accept minutes and fractional seconds");
  bool invalid_time = false;
  try {
    ytwe::timestamp_seconds("1:70");
  } catch (const std::exception &) {
    invalid_time = true;
  }
  check(invalid_time, "malformed section timestamps never silently truncate");
  check(ytwe::remove_query("https://example.org/c?lang=en&xosf=1&sig=a%2Bb",
                           "xosf") == "https://example.org/c?lang=en&sig=a%2Bb",
        "subtitle xosf removal preserves signed query bytes");
  auto translated = ytwe::caption_translation_context(
      R"({"captions":{"playerCaptionsTracklistRenderer":{"captionTracks":[{"baseUrl":"https://example.org/c?lang=en","languageCode":"en","isTranslatable":true}]}}})",
      R"({"captions":{"playerCaptionsTracklistRenderer":{"translationLanguages":[{"languageCode":"fr","languageName":{"simpleText":"French"}}]}}})");
  check(ytwe::subtitle_tracks(translated).size() == 12,
        "WEB translation languages combine with a usable non-WEB caption URL");
  auto existing_subtitle =
      std::filesystem::current_path() / "test-existing-subtitle.vtt";
  {
    std::ofstream file(existing_subtitle);
    file << "WEBVTT\n";
  }
  ytwe::SubtitleTrack existing_track;
  existing_track.url = "invalid://not-requested";
  ytwe::save_subtitle(existing_track, existing_subtitle, "test");
  check(std::filesystem::file_size(existing_subtitle) > 0,
        "existing subtitles skip without making another request");
  std::filesystem::remove(existing_subtitle);
  ytwe::Cache cache;
  cache.directory = std::filesystem::current_path() / "test-native-cache";
  auto payload = std::string(12000, 'x');
  cache.put("fixture", payload);
  check(cache.get("fixture") == payload,
        "persistent cache supports preprocessed JS-size data");
  std::filesystem::remove(cache.directory / ytwe::digest("fixture"));
  std::filesystem::remove(cache.directory);
#ifdef _WIN32
  check(ytwe::windows_quote(L"a b\\") == L"\"a b\\\\\"",
        "Windows trailing-slash argument quoting");
#endif
  std::cout << (failures ? "FAIL" : "PASS") << ": portable YouTube core ("
            << failures << " failures)\n";
  return failures ? 1 : 0;
}
