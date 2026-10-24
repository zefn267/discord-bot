#include "bot.h"
#include "logging.h"
#include "config.h"

#include <spdlog/spdlog.h>


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

    bot discord_bot(*config);

    discord_bot.start();
}
