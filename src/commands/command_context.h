#pragma once

#include <optional>
#include <string>
#include <thread>


class player_manager;

namespace dpp {
class cluster;
} // namespace dpp

class command_registry;

struct command_context {
    dpp::cluster& cluster;
    player_manager& player_mng;
    const command_registry& registry;
};
