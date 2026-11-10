#include "bot.h"
#include "logging.h"
#include "config.h"
#include "audio/decoder.h"


int main() {
    set_settings_logs();

    init_ffmpeg();

    const auto config = get_bot_config();
    if (!config) {
        return 1;
    }

    bot discord_bot(*config);

    discord_bot.start();
}
