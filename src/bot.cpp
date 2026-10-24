#include "bot.h"
#include "logging.h"

#include <spdlog/spdlog.h>


bot::bot(const bot_config& config) : cluster(config.token) {
    cluster.on_log(change_logs_from_dpp);
    cluster.on_slashcommand([this](const dpp::slashcommand_t& event) { handle_slash_command(event); });
    cluster.on_ready([this](const dpp::ready_t& event) { handle_ready(event); });
}

void bot::start() {
    cluster.start(dpp::st_wait);
}

void bot::handle_slash_command(const dpp::slashcommand_t& event) {
    if (event.command.get_command_name() == "ping") {
        spdlog::info(
                "Command /ping was invoked by the user {} (ID: {})", event.command.usr.username, event.command.usr.id);
        event.reply("Pong");
    }
}

void bot::handle_ready(const dpp::ready_t&) {
    if (dpp::run_once<struct register_bot_commands>()) {
        cluster.global_command_create(dpp::slashcommand("ping", "Ping pong!", cluster.me.id));
    }
    spdlog::info("Bot started!");
}
