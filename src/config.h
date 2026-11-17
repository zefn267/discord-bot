#pragma once
#include <optional>
#include <string>


struct bot_config {
    std::string token;
};

std::optional<bot_config> get_bot_config();
