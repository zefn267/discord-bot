#include "soundtrack_info.h"
#include "process/process.h"

#include <vector>
#include <spdlog/spdlog.h>
#include <nlohmann/json.hpp>
#include <cmath>


using json = nlohmann::json;

std::optional<ytdlp::soundtrack_info> ytdlp::fetch_track_info(const std::string& url, std::stop_token token) {
    const std::vector<std::string> args{"yt-dlp", "-J", "--no-playlist", "-f", "bestaudio", "--", url};
    auto result = process::run(args, std::chrono::seconds(30), token);
    if (!result) {
        spdlog::error("Failed to get data in fetch_track_info");
        return std::nullopt;
    }
    if (result->is_stopped) {
        return std::nullopt;
    }
    if (result->is_timeout) {
        spdlog::warn("yt-dlp timed out for '{}'", url);
        return std::nullopt;
    }
    if (result->code != 0) {
        spdlog::error("yt-dlp exited with code {} for '{}': {}", result->code, url, result->stderr_data);
        return std::nullopt;
    }
    json data = json::parse(result->stdout_data, nullptr, false);
    if (data.is_discarded()) {
        spdlog::error("Failed to parse yt-dlp output");
        return std::nullopt;
    }
    if (data["is_live"] == true) {
        spdlog::warn("Live streams are not supported");
        return std::nullopt;
    }
    if (!data["url"].is_string()) {
        spdlog::warn("url is not a string");
        return std::nullopt;
    }
    soundtrack_info info;
    if (data.contains("available_at") && data["available_at"].is_number()) {
        info.available_at = std::chrono::system_clock::time_point{
                std::chrono::seconds{std::llround(data["available_at"].get<double>())}};
    }
    info.stream_url = data["url"];
    info.webpage_url = data.value("webpage_url", std::string{});
    if (data["duration"].is_number()) {
        info.duration = std::chrono::seconds(std::llround(data["duration"].get<double>()));
    }
    info.title = data.value("title", std::string{});
    for (const auto& [header, value] : data["http_headers"].items()) {
        if (value.is_string()) {
            info.http_headers[header] = value;
        }
    }

    return info;
}
