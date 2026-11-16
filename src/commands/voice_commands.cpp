#include "voice_commands.h"
#include "command_registry.h"
#include "command.h"
#include "command_context.h"
#include "audio/decoder.h"
#include "voice/voice_checks.h"
#include "audio/playback.h"
#include "player/player_manager.h"

#include <dpp/cache.h>
#include <dpp/discordclient.h>
#include <dpp/discordvoiceclient.h>
#include <spdlog/spdlog.h>
#include <exception>
#include <thread>


namespace {
void handle_join(const dpp::slashcommand_t& event, const command_context& context) {
    dpp::guild* guild = dpp::find_guild(event.command.guild_id);
    if (!guild) {
        spdlog::warn("Guild not found in cache");
        event.reply("Use this command on server!");
        return;
    }
    if (!event.from()) {
        return;
    }
    auto res = voice::join_user_channel(*guild, *event.from(), context.cluster, event.command.usr.id);
    switch (res) {
        case voice::join_result::user_not_in_voice:
            spdlog::info("User didn't join in channel");
            event.reply(dpp::message("You don't seem to be in a voice channel!").set_flags(dpp::m_ephemeral));
            return;
        case voice::join_result::already_joined:
            spdlog::info("Bot channel already joined");
            event.reply(dpp::message("Don't need to join your channel as i'm already there with you!")
                            .set_flags(dpp::m_ephemeral));
            return;
        case voice::join_result::busy:
            event.reply(dpp::message("I’m already in another channel, use /leave first").set_flags(dpp::m_ephemeral));
            return;
        case voice::join_result::joining:
            event.reply(dpp::message("Joined your voice channel!").set_flags(dpp::m_ephemeral));
            return;
        case voice::join_result::failed:
            event.reply(dpp::message("Failed to join!").set_flags(dpp::m_ephemeral));
    }
}

void handle_leave(const dpp::slashcommand_t& event, const command_context& context) {
    dpp::guild* guild = dpp::find_guild(event.command.guild_id);
    if (!guild) {
        spdlog::warn("Guild not found in cache");
        event.reply("Use this command on server!");
        return;
    }
    if (!event.from()) {
        event.reply(dpp::message("Bot is reconnecting, try again").set_flags(dpp::m_ephemeral));
        return;
    }
    if (voice::is_bot_and_user_in_same_channel(*guild, *event.from(), event.command.usr.id)) {
        context.playback_thread = std::jthread();
        event.reply(dpp::message("Leaving the channel").set_flags(dpp::m_ephemeral));
        context.player_mng.remove(event.command.guild_id);
        event.from()->disconnect_voice(event.command.guild_id);
        spdlog::info("Bot left channel");
        return;
    }
    spdlog::info("Bot and user not on same channel");
    event.reply(dpp::message("You need to be in the same voice channel as the bot").set_flags(dpp::m_ephemeral));
}

void handle_test(const dpp::slashcommand_t& event, const command_context& context) {
    if (!context.testfile_path.has_value()) {
        event.reply(dpp::message("Test audio file is not configured").set_flags(dpp::m_ephemeral));
        return;
    }
    if (!event.from()) {
        return;
    }
    auto connection = event.from()->get_voice(event.command.guild_id);
    if (!connection || !connection->voiceclient || !connection->voiceclient->is_ready()) {
        event.reply("There was an issue with getting the voice channel. Make sure I'm in a voice channel!");
        return;
    }
    decoder dec;
    auto code = dec.open(*context.testfile_path);
    if (code < 0) {
        spdlog::warn("Could not open test file");
        event.reply(dpp::message("Could not open test file").set_flags(dpp::m_ephemeral));
        return;
    }
    event.reply("Playing test file");
    context.playback_thread = std::jthread([dec = std::move(dec), event](std::stop_token token) mutable {
        try {
            dec.set_stop_token(token);
            auto get_client = [&event]() -> dpp::discord_voice_client* {
                auto shard = event.from();
                if (!shard) {
                    return nullptr;
                }
                auto voicecon = shard->get_voice(event.command.guild_id);
                if (!voicecon) {
                    return nullptr;
                }
                if (voicecon->voiceclient && voicecon->voiceclient->is_ready()) {
                    return voicecon->voiceclient.get();
                }
                return nullptr;
            };
            stream_to_voice(dec, get_client, token);
        } catch (const std::exception& e) {
            spdlog::error("Worker error: {}", e.what());
        }
    });
}
} // namespace

void register_voice_commands(command_registry& registry) {
    registry.add_command({.name = "join",
            .description = "Join your voice channel",
            .category = command_category::voice,
            .handler = handle_join});
    registry.add_command({.name = "leave",
            .description = "Leave from channel",
            .category = command_category::voice,
            .handler = handle_leave});
    registry.add_command({.name = "test",
            .description = "test audio file",
            .category = command_category::voice,
            .handler = handle_test});
}
