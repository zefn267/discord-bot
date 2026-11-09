#pragma once

#include <string>
#include <chrono>
#include <optional>
#include <unordered_map>


namespace ytdlp {
struct soundtrack_info {
    std::string title;
    std::chrono::seconds duration{};
    std::string stream_url;
    std::unordered_map<std::string, std::string> http_headers;
    std::string webpage_url;
};

std::optional<soundtrack_info> fetch_track_info(const std::string& url);
} // namespace ytdlp
