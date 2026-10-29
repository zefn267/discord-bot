#pragma once

#include <optional>
#include <dpp/snowflake.h>


namespace dpp {
class guild;
class discord_client;
} // namespace dpp

namespace voice {
std::optional<dpp::snowflake> user_channel(const dpp::guild& guild, dpp::snowflake user_id);

std::optional<dpp::snowflake> bot_channel(dpp::discord_client& client, dpp::snowflake guild_id);

bool is_bot_and_user_in_same_channel(
        const dpp::guild& guild, dpp::discord_client& client, dpp::snowflake user_id);
} // namespace voice