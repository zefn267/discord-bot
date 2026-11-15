#pragma once

#include <functional>
#include <string>
#include <dpp/dispatcher.h>
#include <vector>


enum class command_category { general, voice, music };

struct command_context;

struct command {
    std::string name;
    std::string description;
    command_category category;
    std::function<void(const dpp::slashcommand_t&, const command_context& context)> handler;
    std::vector<dpp::command_option> arguments = {};
};

inline std::string command_category_to_string(const command_category category) {
    switch (category) {
        case command_category::general:
            return "General";
        case command_category::voice:
            return "Voice";
        case command_category::music:
            return "Music";
    }

    return "";
}