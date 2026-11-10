/*
ytdlp_tool — fetches track info for a URL via yt-dlp and prints
title, duration, stream URL, page URL and HTTP headers.

Usage:
  ytdlp_tool <url>

Examples:
  ytdlp_tool "https://www.youtube.com/watch?v=..."
  ytdlp_tool "https://soundcloud.com/artist/track"
*/
#include "ytdlp/soundtrack_info.h"

#include <iostream>
#include <spdlog/spdlog.h>
#include <format>


int main(int argc, char** argv) {
    std::string url = argv[1];
    auto res = ytdlp::fetch_track_info(url);
    if (!res) {
        spdlog::error("Failed to fetch track info");
        return 1;
    }
    std::string info = std::format("title: {}\nduration: {}\nstream_url: {}\nwebpage_url: {}\n", res->title,
            res->duration, res->stream_url, res->webpage_url);
    std::cout << info;
    for (const auto& [header, value] : res->http_headers) {
        std::cout << header << ": " << value << std::endl;
    }
}
