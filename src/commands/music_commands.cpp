#include "music_commands.h"
#include "command_context.h"
#include "command_registry.h"
#include "audio/decoder.h"
#include "audio/playback.h"
#include "voice/voice_checks.h"
#include "ytdlp/soundtrack_info.h"

#include <format>
#include <thread>


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
    event.thinking();
    context.playback_thread = std::jthread([url_string = *track_url, event](std::stop_token token) mutable {
        try {
            auto info = ytdlp::fetch_track_info(url_string);
            if (!info) {
                event.edit_original_response(dpp::message("Failed to get track info!"));
                return;
            }
            auto voice_client = voice::wait_for_voice_client(
                    *event.from(), event.command.guild_id, std::chrono::seconds(10), token);
            if (!voice_client) {
                spdlog::error("Could not connect to the voice channel");
                event.edit_original_response(dpp::message("Could not connect to the voice channel"));
                return;
            }
            decoder dec;
            dec.set_stop_token(token);
            if (dec.open(info->stream_url, info->http_headers) < 0) {
                spdlog::error("Failed to open stream");
                event.edit_original_response(dpp::message("Failed to open stream!"));
                return;
            }
            std::string text = std::format("Now playing: {}", info->title);
            event.edit_original_response(dpp::message(text));
            stream_to_voice(dec, voice_client, token);
        } catch (const std::exception& e) {
            spdlog::error("Worker error: {}", e.what());
        }
    });
}
} // namespace

void register_music_commands(command_registry& registry) {
    registry.add_command({.name = "play",
            .description = "Play a track by URL",
            .category = command_category::music,
            .handler = handle_play,
            .arguments = {dpp::command_option(dpp::co_string, "url", "track url", true)}});
}
