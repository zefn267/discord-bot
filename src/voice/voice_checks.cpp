#include "voice_checks.h"

#include <dpp/discordclient.h>
#include <dpp/guild.h>


std::optional<dpp::snowflake> voice::user_channel(const dpp::guild& guild, dpp::snowflake user_id) {
    auto it = guild.voice_members.find(user_id);
    if (it == guild.voice_members.end()) {
        return std::nullopt;
    }

    return it->second.channel_id;
}

std::optional<dpp::snowflake> voice::bot_channel(dpp::discord_client& client, dpp::snowflake guild_id) {
    auto connection = client.get_voice(guild_id);
    if (connection == nullptr) {
        return std::nullopt;
    }

    return connection->channel_id;
}

bool voice::is_bot_and_user_in_same_channel(
        const dpp::guild& guild, dpp::discord_client& client, dpp::snowflake user_id) {
    auto bot_channel = voice::bot_channel(client, guild.id);
    auto user_channel = voice::user_channel(guild, user_id);
    if (!bot_channel.has_value() || !user_channel.has_value()) {
        return false;
    }

    return *bot_channel == *user_channel;
}
