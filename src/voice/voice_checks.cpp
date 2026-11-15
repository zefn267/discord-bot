#include "voice_checks.h"

#include <dpp/discordclient.h>
#include <dpp/guild.h>
#include <thread>
#include <spdlog/spdlog.h>


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

voice::join_result voice::join_user_channel(
        dpp::guild& guild, dpp::discord_client& client, dpp::cluster& cluster, dpp::snowflake user_id) {
    auto user_channel = voice::user_channel(guild, user_id);
    if (!user_channel) {
        return join_result::user_not_in_voice;
    }
    auto bot_channel = voice::bot_channel(client, guild.id);
    if (bot_channel.has_value()) {
        if (*user_channel == *bot_channel) {
            return join_result::already_joined;
        }
        return join_result::busy;
    }
    if (guild.connect_member_voice(cluster, user_id, false, true)) {
        return join_result::joining;
    }
    spdlog::error("Failed to join user channel");

    return join_result::failed;
}


dpp::discord_voice_client* voice::wait_for_voice_client(
        dpp::discord_client& client, dpp::snowflake guild_id, std::chrono::milliseconds timeout, std::stop_token token) {
    auto deadline = std::chrono::steady_clock::now() + timeout;
    while (true) {
        auto connection = client.get_voice(guild_id);
        if (connection) {
            if (connection->voiceclient) {
                if (connection->voiceclient->is_ready()) {
                    return connection->voiceclient.get();
                }
            }

        }
        if (deadline < std::chrono::steady_clock::now() || token.stop_requested()) {
            return nullptr;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

}