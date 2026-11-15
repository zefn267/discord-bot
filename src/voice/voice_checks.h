#pragma once

#include <optional>
#include <dpp/snowflake.h>


namespace dpp {
class guild;
class discord_client;
} // namespace dpp

namespace voice {
enum class join_result {
    user_not_in_voice,
    already_joined,
    joining,
    busy,
    failed,
};

std::optional<dpp::snowflake> user_channel(const dpp::guild& guild, dpp::snowflake user_id);

std::optional<dpp::snowflake> bot_channel(dpp::discord_client& client, dpp::snowflake guild_id);

bool is_bot_and_user_in_same_channel(const dpp::guild& guild, dpp::discord_client& client, dpp::snowflake user_id);

join_result join_user_channel(
        dpp::guild& guild, dpp::discord_client& client, dpp::cluster& cluster, dpp::snowflake user_id);

dpp::discord_voice_client* wait_for_voice_client(
        dpp::discord_client& client, dpp::snowflake guild_id, std::chrono::milliseconds timeout, std::stop_token token);
} // namespace voice