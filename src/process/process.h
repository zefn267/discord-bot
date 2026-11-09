#pragma once

#include <optional>
#include <string>
#include <vector>


namespace process {
struct result {
    int code = -1;
    std::string stdout_data;
};

std::optional<result> run(const std::vector<std::string>& args);
} // namespace process