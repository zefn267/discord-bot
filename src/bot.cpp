#include "bot.h"
#include "logging.h"
#include "commands/command_registry.h"
#include "commands/general_commands.h"
#include "commands/music_commands.h"
#include "commands/voice_commands.h"

#include <spdlog/spdlog.h>


bot::bot(const bot_config& config)
    : cluster_(config.token), players_(cluster_), testfile_path_(config.audio_file),
      context_{.cluster = cluster_,
              .player_mng = players_,
              .registry = registry_,
              .testfile_path = testfile_path_,
              .playback_thread = test_playback_thread_} {
    cluster_.on_log(change_logs_from_dpp);

    register_general_commands(registry_);
    register_voice_commands(registry_);
    register_music_commands(registry_);

    cluster_.on_slashcommand([this](const dpp::slashcommand_t& event) { handle_slash_command(event); });
    cluster_.on_ready([this](const dpp::ready_t& event) { handle_ready(event); });
    cluster_.on_voice_ready([this](const dpp::voice_ready_t& event) { handle_voice_ready(event); });
}

void bot::start() {
    cluster_.start(dpp::st_wait);
}

void bot::handle_slash_command(const dpp::slashcommand_t& event) const {
    const std::string name = event.command.get_command_name();
    spdlog::info("Command /{} was invoked by {} (ID: {})", name, event.command.usr.username, event.command.usr.id);
    auto cmd = registry_.find_command(name);
    if (cmd != nullptr) {
        cmd->handler(event, context_);
    } else {
        spdlog::warn("Command {} was not found", name);
        event.reply(dpp::message("Unknown command").set_flags(dpp::m_ephemeral));
    }
}

void bot::handle_ready(const dpp::ready_t&) {
    if (dpp::run_once<struct register_bot_commands>()) {
        std::vector<dpp::slashcommand> discord_commands;
        for (auto& [name, cmd] : registry_.get_all_commands()) {
            dpp::slashcommand definition(cmd.name, cmd.description, cluster_.me.id);
            for (const auto& arg : cmd.arguments) {
                definition.add_option(arg);
            }
            discord_commands.push_back(std::move(definition));
        }
        cluster_.global_bulk_command_create(discord_commands);
    }

    spdlog::info("Bot started!");
}

void bot::handle_voice_ready(const dpp::voice_ready_t& event) {
    dpp::discord_voice_client* voice_client = event.voice_client;
    if (voice_client && voice_client->is_ready()) {
        spdlog::info("Bot is ready for voice! Channel_id: {}", voice_client->channel_id);
    }
}
