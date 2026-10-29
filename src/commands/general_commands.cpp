#include "command_registry.h"
#include "general_commands.h"
#include "command.h"
#include "command_context.h"

#include <dpp/dpp.h>
#include <spdlog/spdlog.h>
#include <unordered_map>
#include <array>
#include <format>


namespace {
void handle_ping(const dpp::slashcommand_t& event, const command_context&) {
    event.reply("Pong");
}

void handle_help(const dpp::slashcommand_t& event, const command_context& context) {
    std::array categories{command_category::general, command_category::voice, command_category::music};
    std::unordered_map<command_category, std::string> grouped_text;
    const auto& commands = context.registry.get_all_commands();
    for (const auto& [name, command] : commands) {
        grouped_text[command.category] += std::format("`/{}` - {}\n", command.name, command.description);
    }
    std::string full_description = "The main commands for this server:\n\n";
    for (const auto& group_name : categories) {
        if (auto it = grouped_text.find(group_name); it != grouped_text.end()) {
            full_description += "**" + command_category_to_string(group_name) + "**\n";
            full_description += it->second + "\n";
        }
    }
    const dpp::embed embed =
            dpp::embed().set_color(0x3498db).set_title("🎵 Music bot help").set_description(full_description);
    dpp::message msg(event.command.channel_id, embed);
    event.reply(msg.set_flags(dpp::m_ephemeral));
}
} // namespace

void register_general_commands(command_registry& registry) {
    registry.add_command({.name = "ping",
            .description = "Check bot",
            .category = command_category::general,
            .handler = handle_ping});
    registry.add_command({.name = "help",
            .description = "Show available commands",
            .category = command_category::general,
            .handler = handle_help});
}
