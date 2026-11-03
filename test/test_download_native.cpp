#include "we_download.h"
#include <iostream>
int main(int argc, char **argv) {
  try {
    if (argc < 4)
      return 2;
    ytfast::StreamFormat f;
    f.url = argv[1];
    f.format_id = "probe";
    f.content_length = std::stoll(argv[2]);
    if (argc > 4 && std::string(argv[4]) == "fragments") {
      for (int i = 0; i < 3; ++i)
        f.fragments.push_back({f.url + "/" + std::to_string(i), "", 0});
      f.protocol = "http_dash_segments";
    }
    ytwe::download_file(f, std::filesystem::u8path(argv[3]));
    std::cout << "download complete\n";
    return 0;
  } catch (const std::exception &e) {
    std::cerr << e.what() << '\n';
    return 1;
  }
}
