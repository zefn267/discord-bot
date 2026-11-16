#include "player_manager.h"


player_manager::player_manager(dpp::cluster& cluster) : cluster_(cluster) {}

std::shared_ptr<guild_player> player_manager::find_player(dpp::snowflake guild_id) {
    std::lock_guard lock(mx_);
    auto it = players_.find(guild_id);
    if (it == players_.end()) {
        return nullptr;
    }
    return it->second;
}

std::shared_ptr<guild_player> player_manager::get_or_create(dpp::snowflake guild_id) {
    std::lock_guard lock(mx_);
    auto it = players_.find(guild_id);
    if (it != players_.end()) {
        return it->second;
    }
    players_[guild_id] = std::make_shared<guild_player>(cluster_, guild_id);
    return players_[guild_id];
}

bool player_manager::remove(dpp::snowflake guild_id) {
    std::shared_ptr<guild_player> local = nullptr;
    {
        std::lock_guard lock(mx_);
        auto it = players_.find(guild_id);
        if (it != players_.end()) {
            local = std::move(it->second);
            players_.erase(it);
            return true;
        }
    }
    return false;
}
