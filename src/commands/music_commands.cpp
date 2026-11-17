#include "music_commands.h"
#include "command_context.h"
#include "command_registry.h"
#include "audio/decoder.h"
#include "audio/playback.h"
#include "player/player_manager.h"
#include "voice/voice_checks.h"


namespace {
constexpr size_t MAX_OUTPUT_TRACKS = 10;

void handle_play(const dpp::slashcommand_t& event, const command_context& context) {
    auto url = event.get_parameter("url");
    auto track_url = std::get_if<std::string>(&url);
    if (!track_url) {
        event.reply(dpp::message("bad request").set_flags(dpp::m_ephemeral));
        return;
    }
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
    auto res = voice::join_user_channel(*guild, *event.from(), context.cluster, event.command.usr.id);
    switch (res) {
        case voice::join_result::busy:
            event.reply(dpp::message("I’m already in another channel, use /leave first").set_flags(dpp::m_ephemeral));
            return;
        case voice::join_result::user_not_in_voice:
            event.reply(dpp::message("You don't seem to be in a voice channel!").set_flags(dpp::m_ephemeral));
            return;
        case voice::join_result::failed:
            event.reply(dpp::message("Failed to join!").set_flags(dpp::m_ephemeral));
            return;
        case voice::join_result::joining:
            break;
        case voice::join_result::already_joined:
            break;
    }
    auto player = context.player_mng.get_or_create(event.command.guild_id);
    track t{.webpage_url = *track_url, .user_id = event.command.usr.id, .text_id = event.command.channel_id};
    player->enqueue(std::move(t));
    event.reply("Added to queue");
}

void handle_skip(const dpp::slashcommand_t& event, const command_context& context) {
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
        auto player = context.player_mng.find_player(event.command.guild_id);
        if (player) {
            if (player->skip()) {
                event.reply("Skipped");
                return;
            }
            event.reply("Nothing is playing");
            return;
        }
        event.reply("Nothing is playing");
        return;
    }
    event.reply(dpp::message("You need to be in the same voice channel as the bot").set_flags(dpp::m_ephemeral));
}

void handle_clear(const dpp::slashcommand_t& event, const command_context& context) {
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
        auto player = context.player_mng.find_player(event.command.guild_id);
        if (player) {
            player->clear();
            event.reply("Queue cleared");
            return;
        }
        event.reply("Nothing in queue");
        return;
    }
    event.reply(dpp::message("You need to be in the same voice channel as the bot").set_flags(dpp::m_ephemeral));
}

void handle_stop(const dpp::slashcommand_t& event, const command_context& context) {
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
        auto player = context.player_mng.find_player(event.command.guild_id);
        if (player) {
            player->stop();
            event.reply("Stopped");
            return;
        }
        event.reply("Nothing is playing");
        return;
    }
    event.reply(dpp::message("You need to be in the same voice channel as the bot").set_flags(dpp::m_ephemeral));
}

void handle_queue(const dpp::slashcommand_t& event, const command_context& context) {
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
        auto player = context.player_mng.find_player(event.command.guild_id);
        if (player) {
            auto v = player->queue_snapshot();
            if (v.empty()) {
                event.reply("Queue is empty");
                return;
            }
            std::string res = "The list of tracks this server:\n\n";
            for (size_t i = 0; i < MAX_OUTPUT_TRACKS && i < v.size(); ++i) {
                res += std::format("{}. `{}`\n", i + 1, v[i].webpage_url);
            }
            if (v.size() > MAX_OUTPUT_TRACKS) {
                res += std::format("... and {} more", v.size() - MAX_OUTPUT_TRACKS);
            }
            const dpp::embed embed =
                    dpp::embed().set_color(0x3498db).set_title("🎵 List of tracks").set_description(res);
            dpp::message msg(event.command.channel_id, embed);
            event.reply(msg);
            return;
        }
        event.reply("Queue is empty");
        return;
    }
    event.reply(dpp::message("You need to be in the same voice channel as the bot").set_flags(dpp::m_ephemeral));
}
} // namespace

void register_music_commands(command_registry& registry) {
    registry.add_command({.name = "play",
            .description = "Play a track by URL",
            .category = command_category::music,
            .handler = handle_play,
            .arguments = {dpp::command_option(dpp::co_string, "url", "track url", true)}});
    registry.add_command(
            {.name = "skip", .description = "Skip track", .category = command_category::music, .handler = handle_skip});
    registry.add_command({.name = "clear",
            .description = "Clear queue",
            .category = command_category::music,
            .handler = handle_clear});
    registry.add_command({.name = "stop",
            .description = "Stop music and clear queue",
            .category = command_category::music,
            .handler = handle_stop});
    registry.add_command({.name = "queue",
            .description = "List of tracks",
            .category = command_category::music,
            .handler = handle_queue});
}
