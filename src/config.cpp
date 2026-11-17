#include "config.h"
#include <spdlog/spdlog.h>
#include <cstdlib>


std::optional<bot_config> get_bot_config() {
    bot_config cfg{};

    const char* token = std::getenv("BOT_TOKEN");
    if (token == nullptr) {
        spdlog::error("Can't get BOT_TOKEN from env var");
        return std::nullopt;
    }
    cfg.token = token;

    return cfg;
}
