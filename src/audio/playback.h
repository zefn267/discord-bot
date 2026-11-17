#pragma once

#include "decoder.h"

#include <functional>
#include <dpp/discordclient.h>


enum class stream_result {
    ended,
    stopped,
    lost,
    error,
};

stream_result stream_to_voice(
        decoder& dec, const std::function<dpp::discord_voice_client*()>& get_client, const std::stop_token& token);
