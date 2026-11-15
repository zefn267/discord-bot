#pragma once

#include "decoder.h"

#include <dpp/discordclient.h>

void stream_to_voice(decoder& dec, dpp::discord_voice_client* voice_conn, const std::stop_token& token);
