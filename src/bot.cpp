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
    commands["join"] = {.name = "join",
            .description = "Join to channel",
            .category = "Voice",
            .handler = std::bind_front(&bot::handle_join, this)};
    commands["leave"] = {.name = "leave",
            .description = "Leave from channel",
            .category = "Voice",
            .handler = std::bind_front(&bot::handle_leave, this)};

    cluster.on_slashcommand([this](const dpp::slashcommand_t& event) { handle_slash_command(event); });
    cluster.on_ready([this](const dpp::ready_t& event) { handle_ready(event); });
    cluster.on_voice_ready([this](const dpp::voice_ready_t& event) { handle_voice_ready(event); });
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
    std::vector<std::string> group_order = {"Music", "Information", "Voice"};

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

void bot::handle_join(const dpp::slashcommand_t& event) {
    spdlog::info("Command /join was invoked by the user {} (ID: {})", event.command.usr.username, event.command.usr.id);

    dpp::guild* guild = dpp::find_guild(event.command.guild_id);
    if (!guild) {
        spdlog::warn("Guild is not find");
        event.reply("Use this command on server!");
        return;
    }
    auto current_vc = event.from()->get_voice(event.command.guild_id);

    bool join_vc = true;

    if (current_vc) {
        auto user_vc = guild->voice_members.find(event.command.get_issuing_user().id);
        if (user_vc != guild->voice_members.end() && current_vc->channel_id == user_vc->second.channel_id) {
            join_vc = false;
            spdlog::info("Bot in right channel");
        } else {
            spdlog::info("Bot on different channel");
            event.reply(dpp::message("I’m already in another channel, use /leave first").set_flags(dpp::m_ephemeral));
            return;
        }
    }
    if (join_vc) {
        if (!guild->connect_member_voice(*event.owner, event.command.get_issuing_user().id)) {
            event.reply(dpp::message("You don't seem to be in a voice channel!").set_flags(dpp::m_ephemeral));
            return;
        }
        event.reply(dpp::message("Joined your voice channel!").set_flags(dpp::m_ephemeral));
    } else {
        event.reply(dpp::message("Don't need to join your channel as i'm already there with you!")
                        .set_flags(dpp::m_ephemeral));
    }
}

void bot::handle_voice_ready(const dpp::voice_ready_t& event) {
    dpp::discord_voice_client* voice_client = event.voice_client;
    if (voice_client && voice_client->is_ready()) {
        spdlog::info("Bot is ready for voice! Channel_id: {}", voice_client->channel_id);
    }
}

void bot::handle_leave(const dpp::slashcommand_t& event) {
    dpp::guild* guild = dpp::find_guild(event.command.guild_id);
    if (!guild) {
        spdlog::warn("Guild is not find");
        event.reply("Use this command on server!");
        return;
    }
    auto current_vc = event.from()->get_voice(event.command.guild_id);
    if (current_vc) {
        auto user_vc = guild->voice_members.find(event.command.get_issuing_user().id);
        if (user_vc != guild->voice_members.end() && current_vc->channel_id == user_vc->second.channel_id) {
            event.from()->disconnect_voice(event.command.guild_id);
            spdlog::info("Bot left channel");
            event.reply(dpp::message("Bot left the channel").set_flags(dpp::m_ephemeral));
            return;
        } else {
            spdlog::info("User send /leave but he on different channel");
            event.reply(
                    dpp::message("You need to be in the same voice channel as the bot").set_flags(dpp::m_ephemeral));
            return;
        }
    }
    event.reply(dpp::message("You need to be in the same voice channel as the bot").set_flags(dpp::m_ephemeral));
}
