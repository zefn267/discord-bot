#pragma once

#include <optional>
#include <string>
#include <vector>
#include <chrono>
#include <stop_token>


namespace process {
struct result {
    int code = -1;
    std::string stdout_data;
    std::string stderr_data;
    bool is_timeout = false;
    bool is_stopped = false;
};

std::optional<result> run(const std::vector<std::string>& args,
        std::chrono::milliseconds timeout = std::chrono::seconds(30), std::stop_token token = {});
} // namespace process