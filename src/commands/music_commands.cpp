#include "music_commands.h"
#include "command_context.h"
#include "command_registry.h"
#include "audio/decoder.h"
#include "audio/playback.h"
#include "player/player_manager.h"
#include "voice/voice_checks.h"


namespace {
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
    track t {.webpage_url = *track_url, .user_id = event.command.usr.id, .text_id = event.command.channel_id };
    player->enqueue(std::move(t));
    event.reply("Added to queue");
}
} // namespace

void register_music_commands(command_registry& registry) {
    registry.add_command({.name = "play",
            .description = "Play a track by URL",
            .category = command_category::music,
            .handler = handle_play,
            .arguments = {dpp::command_option(dpp::co_string, "url", "track url", true)}});
}
