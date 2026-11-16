#include "guild_player.h"

#include "audio/decoder.h"
#include "audio/playback.h"
#include "voice/voice_checks.h"
#include "ytdlp/soundtrack_info.h"

#include <dpp/cache.h>
#include <dpp/cluster.h>
#include <dpp/guild.h>
#include <spdlog/spdlog.h>
#include <format>


namespace {
constexpr int MAX_OPEN_ATTEMPTS = 4;
} // namespace

guild_player::guild_player(dpp::cluster& cluster, dpp::snowflake guild_id)
    : cluster_(cluster), guild_id_(guild_id), thread_([this](std::stop_token token) { run(token); }) {}

void guild_player::run(std::stop_token token) {
    while (!token.stop_requested()) {
        auto cur_track = track_queue_.pop(token);
        if (!cur_track) {
            break;
        }
        play_track(std::move(cur_track.value()), token);
    }
}

void guild_player::enqueue(track track) {
    track_queue_.push(std::move(track));
}

void guild_player::play_track(track current_track, std::stop_token token) {
    std::stop_source source;
    {
        std::lock_guard lock(state_mx_);
        current_track_ = current_track;
        current_track_stop_ = source;
    }
    std::stop_callback link(token, [&]() { static_cast<void>(source.request_stop()); });
    try {
        stream_track(current_track, source.get_token());
    } catch (const std::exception& e) {
        spdlog::error("Worker error: {}", e.what());
    }
    {
        std::lock_guard lock(state_mx_);
        current_track_ = std::nullopt;
        current_track_stop_ = std::nullopt;
    }
}

void guild_player::stream_track(const track& current_track, std::stop_token token) {
    auto info = ytdlp::fetch_track_info(current_track.webpage_url);
    if (!info) {
        write_in_channel(current_track.text_id, std::format("Could not load track: {}", current_track.webpage_url));
        return;
    }
    std::optional<decoder> dec;
    for (int i = 0; i < MAX_OPEN_ATTEMPTS; ++i) {
        if (token.stop_requested()) {
            return;
        }
        decoder d;
        d.set_stop_token(token);
        if (d.open(info->stream_url, info->http_headers) < 0) {
            spdlog::warn("Failed to open stream (Attempt {}/{}, retrying)", i + 1, MAX_OPEN_ATTEMPTS);
            if (i == MAX_OPEN_ATTEMPTS - 1) {
                spdlog::error("Too many attempts");
                write_in_channel(current_track.text_id, "Could not open stream");
                return;
            }
        } else {
            dec = std::move(d);
            break;
        }
        info = ytdlp::fetch_track_info(current_track.webpage_url);
        if (!info) {
            write_in_channel(current_track.text_id, std::format("Could not load track: {}", current_track.webpage_url));
            return;
        }
    }
    auto voice_client = find_voice_client(token);
    if (!voice_client) {
        if (!token.stop_requested()) {
            write_in_channel(current_track.text_id, "Could not connect to the voice channel");
            return;
        }
    }
    write_in_channel(current_track.text_id, std::format("Now playing: {}", info->title));
    stream_to_voice(*dec, voice_client, token);
}

dpp::discord_voice_client* guild_player::find_voice_client(std::stop_token token) {
    auto guild = dpp::find_guild(guild_id_);
    if (!guild) {
        return nullptr;
    }
    auto shard_id = guild->shard_id;
    auto shard = cluster_.get_shard(shard_id);
    if (!shard) {
        return nullptr;
    }
    auto voice_client = voice::wait_for_voice_client(*shard, guild_id_, std::chrono::seconds(10), token);
    if (!voice_client) {
        spdlog::error("Could not connect to the voice channel");
        return nullptr;
    }
    return voice_client;
}

void guild_player::write_in_channel(dpp::snowflake text_id, const std::string& text) {
    cluster_.message_create(dpp::message(text_id, text));
}
