#include "logging.h"
#include "config.h"

#include <spdlog/spdlog.h>
#include <dpp/dpp.h>


extern "C" {
#include <libavutil/avutil.h>
}


static void print_version() {
    auto version = av_version_info();
    spdlog::info("FFmpeg version: {}", version);
}

int main() {
    set_settings_logs();

    print_version();

    const auto config = get_bot_config();
    if (!config) {
        return 1;
    }

    dpp::cluster bot(config->token);

    bot.on_log(change_logs_from_dpp);

    bot.on_slashcommand([](const dpp::slashcommand_t& event) {
        if (event.command.get_command_name() == "ping") {
            spdlog::info("Command /ping was invoked by the user {} (ID: {})", event.command.usr.username,
                    event.command.usr.id);
            event.reply("Pong");
        }
    });

    bot.on_ready([&bot](const dpp::ready_t&) {
        if (dpp::run_once<struct register_bot_commands>()) {
            bot.global_command_create(dpp::slashcommand("ping", "Ping pong!", bot.me.id));
        }
        spdlog::info("Bot started!");
    });

    bot.start(dpp::st_wait);
}
