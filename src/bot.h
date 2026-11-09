#pragma once
#include "config.h"
#include "commands/command_registry.h"
#include "commands/command_context.h"

#include <thread>
#include <dpp/dpp.h>


class bot {
public:
    explicit bot(const bot_config& config);

    bot(const bot&) = delete;

    bot& operator=(const bot&) = delete;

    void start();

private:
    dpp::cluster cluster_;
    command_registry registry_;
    std::optional<std::string> testfile_path_;
    std::jthread test_playback_thread_;
    command_context context_;

    void handle_slash_command(const dpp::slashcommand_t& event) const;

    void handle_ready(const dpp::ready_t&);

    void handle_voice_ready(const dpp::voice_ready_t& event);
};
