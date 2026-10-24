#include "bot.h"
#include "logging.h"

#include <spdlog/spdlog.h>
#include <format>


bot::bot(const bot_config& config) : cluster(config.token) {
    cluster.on_log(change_logs_from_dpp);

    commands["ping"] = {.name = "ping",
            .description = "Check bot",
            .category = "Information",
            .handler = std::bind_front(&bot::handle_ping, this)};
    commands["help"] = {.name = "help",
            .description = "Show available commands",
            .category = "Information",
            .handler = std::bind_front(&bot::handle_help, this)};
    cluster.on_slashcommand([this](const dpp::slashcommand_t& event) { handle_slash_command(event); });
    cluster.on_ready([this](const dpp::ready_t& event) { handle_ready(event); });
}

void bot::start() {
    cluster.start(dpp::st_wait);
}

void bot::handle_slash_command(const dpp::slashcommand_t& event) {
    const std::string name = event.command.get_command_name();
    auto it = commands.find(name);
    if (it != commands.end()) {
        it->second.handler(event);
    } else {
        spdlog::warn("Command {} was not found", name);
        event.reply(dpp::message("Unknown command").set_flags(dpp::m_ephemeral));
    }
}

void bot::handle_ready(const dpp::ready_t&) {
    if (dpp::run_once<struct register_bot_commands>()) {
        std::vector<dpp::slashcommand> discord_commands;
        for (auto& [name, cmd] : commands) {
            discord_commands.emplace_back(cmd.name, cmd.description, cluster.me.id);
        }
        cluster.global_bulk_command_create(discord_commands);
    }

    spdlog::info("Bot started!");
}

void bot::handle_ping(const dpp::slashcommand_t& event) {
    spdlog::info("Command /ping was invoked by the user {} (ID: {})", event.command.usr.username, event.command.usr.id);
    event.reply("Pong");
}

void bot::handle_help(const dpp::slashcommand_t& event) {
    spdlog::info("Command /help was invoked by the user {} (ID: {})", event.command.usr.username, event.command.usr.id);

    std::unordered_map<std::string, std::string> grouped_text;
    std::vector<std::string> group_order = {"Music", "Information"};

    for (const auto& [name, command] : commands) {
        grouped_text[command.category] += std::format("`/{}` - {}\n", command.name, command.description);
    }
    std::string full_description = "The main commands for this server:\n\n";
    for (const auto& group_name : group_order) {
        if (auto it = grouped_text.find(group_name); it != grouped_text.end()) {
            full_description += "**" + group_name + "**\n";
            full_description += it->second + "\n";
        }
    }

    const dpp::embed embed =
            dpp::embed().set_color(0x3498db).set_title("🎵 Music bot help").set_description(full_description);
    dpp::message msg(event.command.channel_id, embed);
    event.reply(msg.set_flags(dpp::m_ephemeral));
}
