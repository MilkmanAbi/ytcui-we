#include "yt_youtube.h"
#include <iostream>
int main() {
  int failures = 0;
  auto check = [&](bool ok, const char *text) {
    if (!ok) {
      ++failures;
      std::cerr << "FAIL: " << text << '\n';
    }
  };
  auto file = std::filesystem::current_path() / "test-native-cookies.txt";
  {
    std::ofstream f(file);
    for (auto name : {"SAPISID", "__Secure-1PAPISID", "__Secure-3PAPISID"})
      f << ".youtube.com\tTRUE\t/\tTRUE\t0\t" << name << "\tfake-test-cookie\n";
  }
  bool vision = false;
  size_t calls = 0;
  ytwe::Youtube yt([&](const std::string &url, const std::string &body,
                       const std::vector<std::string> &headers) {
    auto has = [&](const std::string &prefix) {
      return std::any_of(
          headers.begin(), headers.end(),
          [&](const std::string &h) { return h.rfind(prefix, 0) == 0; });
    };
    if (body.empty() && url.find("example.org") != std::string::npos) {
      check(!has("Cookie:") && !has("Authorization:"),
            "credentials are not sent to manifest/third-party hosts");
      return std::string("public");
    }
    if (body.empty())
      return std::string(
          R"(<script>ytcfg.set({"INNERTUBE_CONTEXT":{"client":{"clientName":"WEB","clientVersion":"test"}},"INNERTUBE_API_KEY":"key","STS":11111,"SESSION_INDEX":2,"DATASYNC_ID":"DELEGATE||USER","LOGGED_IN":true});var ytInitialData = {};var ytInitialPlayerResponse = {"videoDetails":{"videoId":"aqz-KE-bpKQ"}};</script>)");
    ++calls;
    if (vision)
      check(!has("Cookie:") && !has("Authorization:"),
            "VisionOS requests omit browser-session credentials");
    else {
      check(has("Cookie:") && has("Authorization:"),
            "WEB/TV requests include explicit session");
      check(has("X-Goog-AuthUser: 2") && has("X-Goog-PageId: DELEGATE"),
            "account/delegated session headers");
      auto auth =
          std::find_if(headers.begin(), headers.end(), [](const auto &h) {
            return h.rfind("Authorization:", 0) == 0;
          });
      if (auth != headers.end())
        check(auth->find("SAPISID1PHASH") != std::string::npos &&
                  auth->find("SAPISID3PHASH") != std::string::npos &&
                  auth->find("_u") != std::string::npos,
              "three SID schemes include user binding");
    }
    if (url.find("/player?") != std::string::npos && !vision)
      check(ytwe::str(yj::path(yj::parse(body), "serviceIntegrityDimensions",
                               "poToken")) == "synthetic-token",
            "player token is attached to the selected client");
    return std::string(
        R"({"playabilityStatus":{"status":"OK"},"videoDetails":{"videoId":"aqz-KE-bpKQ"}})");
  });
  yt.cookies(file.string());
  yt.bootstrap("aqz-KE-bpKQ");
  yt.api("next", "{}");
  yt.set_po_token("tv_samsung.player+synthetic-token");
  yt.player_response("aqz-KE-bpKQ", "tv_samsung");
  vision = true;
  yt.player_response("aqz-KE-bpKQ", "visionos");
  vision = false;
  yt.api("next", "{}");
  yt.get_url("https://example.org/public");
  check(calls == 4, "mock authenticated requests completed");
  std::filesystem::remove(file);
  std::cout << (failures ? "FAIL" : "PASS") << ": scoped authentication ("
            << failures << " failures)\n";
  return failures ? 1 : 0;
}
