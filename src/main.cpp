#include "bot.h"
#include "logging.h"
#include "config.h"
#include "audio/decoder.h"

#include <spdlog/spdlog.h>


int main() {
    set_settings_logs();

    init_ffmpeg();

    const auto config = get_bot_config();
    if (!config) {
        return 1;
    }

    sigset_t signals;
    sigemptyset(&signals);
    sigaddset(&signals, SIGINT);
    sigaddset(&signals, SIGTERM);
    if (pthread_sigmask(SIG_BLOCK, &signals, nullptr) != 0) {
        spdlog::error("Failed to block signals");
        return 1;
    }

    bot discord_bot(*config);
    std::jthread signal_watcher([&discord_bot, signals]() {
        int sig = 0;
        sigwait(&signals, &sig);
        spdlog::info("Received signal {}, shutting down", sig);
        discord_bot.stop();
    });
    discord_bot.start();
    pthread_kill(signal_watcher.native_handle(), SIGTERM);

}
