#pragma once

#include "command.h"

#include <unordered_map>
#include <string>


class command_registry {
public:
    void add_command(command cmd);

    const command* find_command(const std::string& name) const;

    const std::unordered_map<std::string, command>& get_all_commands() const;

private:
    std::unordered_map<std::string, command> commands;
};
