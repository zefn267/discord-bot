#pragma once

#include <optional>
#include <string>


namespace dpp {
class cluster;
} // namespace dpp

class command_registry;

struct command_context {
    dpp::cluster& cluster;
    const command_registry& registry;
    const std::optional<std::string>& testfile_path;
};
