#pragma once

#include "track_queue.h"

#include <thread>
#include <dpp/snowflake.h>


namespace dpp {
class cluster;
class discord_voice_client;
} // namespace dpp

class guild_player {
public:
    guild_player(dpp::cluster& cluster, dpp::snowflake guild_id);

    guild_player(const guild_player&) = delete;

    guild_player& operator=(const guild_player&) = delete;

    void enqueue(track track);

    bool skip();

    void clear();

    void stop();

    std::vector<track> queue_snapshot() const;

private:
    dpp::cluster& cluster_;
    dpp::snowflake guild_id_;
    track_queue track_queue_;
    std::mutex state_mx_;
    std::optional<std::stop_source> current_track_stop_;
    std::optional<track> current_track_;
    std::jthread thread_;

    void run(std::stop_token token);

    void play_track(track current_track, std::stop_token token);

    dpp::discord_voice_client* find_voice_client(std::stop_token token);

    void stream_track(const track& current_track, std::stop_token token);

    void write_in_channel(dpp::snowflake text_id, const std::string& text);
};