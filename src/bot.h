#pragma once
#include "config.h"
#include "commands.h"
#include <dpp/dpp.h>
#include <unordered_map>


class bot {
public:
    explicit bot(const bot_config& config);

    bot(const bot&) = delete;

    bot& operator=(const bot&) = delete;

    void start();

private:
    dpp::cluster cluster;
    std::unordered_map<std::string, command> commands;
    std::optional<std::string> test_file_path;

    void handle_slash_command(const dpp::slashcommand_t& event);

    void handle_ready(const dpp::ready_t&);

    void handle_ping(const dpp::slashcommand_t& event);

    void handle_help(const dpp::slashcommand_t& event);

    void handle_join(const dpp::slashcommand_t& event);

    void handle_voice_ready(const dpp::voice_ready_t& event);

    void handle_leave(const dpp::slashcommand_t& event);

    void handle_test(const dpp::slashcommand_t& event);
};
