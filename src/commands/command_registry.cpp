#include "command_registry.h"
#include "command.h"
#include <utility>


void command_registry::add_command(command cmd) {
    std::string name = cmd.name;
    commands.insert({name, std::move(cmd)});
}

const command* command_registry::find_command(const std::string& name) const {
    const auto it = commands.find(name);
    if (it == commands.end()) {
        return nullptr;
    }

    return &it->second;
}

const std::unordered_map<std::string, command>& command_registry::get_all_commands() const {
    return commands;
}
