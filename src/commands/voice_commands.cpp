#include "voice_commands.h"
#include "command_registry.h"
#include "command.h"
#include "command_context.h"
#include "audio/decoder.h"
#include "voice/voice_checks.h"

#include <dpp/cache.h>
#include <dpp/discordclient.h>
#include <dpp/discordvoiceclient.h>
#include <spdlog/spdlog.h>
#include <exception>
#include <chrono>
#include <thread>


namespace {
void handle_join(const dpp::slashcommand_t& event, const command_context& context) {
    dpp::guild* guild = dpp::find_guild(event.command.guild_id);
    if (!guild) {
        spdlog::warn("Guild not found in cache");
        event.reply("Use this command on server!");
        return;
    }
    auto user_channel = voice::user_channel(*guild, event.command.usr.id);
    if (!user_channel) {
        spdlog::info("User didn't join in channel");
        event.reply(dpp::message("You don't seem to be in a voice channel!").set_flags(dpp::m_ephemeral));
        return;
    }
    auto bot_channel = voice::bot_channel(*event.from(), guild->id);
    if (bot_channel.has_value()) {
        if (*user_channel == *bot_channel) {
            spdlog::info("Bot channel already joined");
            event.reply(dpp::message("Don't need to join your channel as i'm already there with you!")
                            .set_flags(dpp::m_ephemeral));
            return;
        }
        event.reply(dpp::message("I’m already in another channel, use /leave first").set_flags(dpp::m_ephemeral));
        return;
    }
    if (guild->connect_member_voice(context.cluster, event.command.usr.id)) {
        event.reply(dpp::message("Joined your voice channel!").set_flags(dpp::m_ephemeral));
        return;
    }
    event.reply(dpp::message("You don't seem to be in a voice channel!").set_flags(dpp::m_ephemeral));
}

void handle_leave(const dpp::slashcommand_t& event, const command_context&) {
    dpp::guild* guild = dpp::find_guild(event.command.guild_id);
    if (!guild) {
        spdlog::warn("Guild not found in cache");
        event.reply("Use this command on server!");
        return;
    }
    if (voice::is_bot_and_user_in_same_channel(*guild, *event.from(), event.command.usr.id)) {
        event.from()->disconnect_voice(event.command.guild_id);
        spdlog::info("Bot left channel");
        event.reply(dpp::message("Bot left the channel").set_flags(dpp::m_ephemeral));
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
    context.test_playback_thread = std::jthread(
            [dec = std::move(dec), voice_client = connection->voiceclient.get()](std::stop_token token) mutable {
                try {
                    std::vector<int16_t> pcm;
                    std::vector<int16_t> buf;
                    while (!token.stop_requested()) {
                        while (voice_client->get_secs_remaining() > 2 && !token.stop_requested()) {
                            std::this_thread::sleep_for(std::chrono::milliseconds(100));
                        }
                        auto result = dec.read_pcm(pcm);
                        if (result == read_result::error) {
                            spdlog::error("Failed to read pcm");
                            break;
                        }
                        if (result == read_result::data) {
                            buf.insert(buf.end(), pcm.begin(), pcm.end());
                        }
                        if (result == read_result::end_of_stream) {
                            if (!buf.empty()) {
                                buf.resize(dpp::send_audio_raw_max_length / sizeof(int16_t));
                                voice_client->send_audio_raw(
                                        reinterpret_cast<uint16_t*>(buf.data()), dpp::send_audio_raw_max_length);
                                buf.clear();
                            }
                            break;
                        }
                        auto bytes = buf.size() * sizeof(int16_t);
                        while (bytes >= dpp::send_audio_raw_max_length) {
                            voice_client->send_audio_raw(
                                    reinterpret_cast<uint16_t*>(buf.data()), dpp::send_audio_raw_max_length);
                            buf.erase(buf.begin(), buf.begin() + dpp::send_audio_raw_max_length / sizeof(int16_t));
                            bytes -= dpp::send_audio_raw_max_length;
                        }
                    }
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
