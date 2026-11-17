#pragma once

#include <string>
#include <chrono>
#include <optional>
#include <unordered_map>
#include <stop_token>


namespace ytdlp {
struct soundtrack_info {
    std::string title;
    std::chrono::seconds duration{};
    std::string stream_url;
    std::unordered_map<std::string, std::string> http_headers;
    std::string webpage_url;
    std::optional<std::chrono::system_clock::time_point> available_at;
};

std::optional<soundtrack_info> fetch_track_info(const std::string& url, std::stop_token token = {});
} // namespace ytdlp
