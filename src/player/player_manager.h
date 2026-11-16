#pragma once

#include "guild_player.h"


#include <mutex>
#include <unordered_map>
#include <memory>


class player_manager {
public:
    player_manager(dpp::cluster& cluster);

    std::shared_ptr<guild_player> get_or_create(dpp::snowflake guild_id);

    std::shared_ptr<guild_player> find_player(dpp::snowflake guild_id);

    void remove(dpp::snowflake guild_id);

private:
    dpp::cluster& cluster_;
    std::unordered_map<dpp::snowflake, std::shared_ptr<guild_player>> players_;
    std::mutex mx_;
};