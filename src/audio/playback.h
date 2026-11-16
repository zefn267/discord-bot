#pragma once

#include "decoder.h"

#include <functional>
#include <dpp/discordclient.h>

void stream_to_voice(
        decoder& dec, const std::function<dpp::discord_voice_client*()>& get_client, const std::stop_token& token);
