#pragma once
#include <functional>
#include <string>
#include <dpp/dispatcher.h>


struct command {
    std::string name;
    std::string description;
    std::string category;
    std::function<void(const dpp::slashcommand_t&)> handler;
};