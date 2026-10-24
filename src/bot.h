#pragma once
#include <dpp/dpp.h>
#include "config.h"


class bot {
public:
    explicit bot(const bot_config& config);

    bot(const bot&) = delete;

    bot& operator=(const bot&) = delete;

    void start();

private:
    dpp::cluster cluster;

    void handle_slash_command(const dpp::slashcommand_t& event);

    void handle_ready(const dpp::ready_t&);
};