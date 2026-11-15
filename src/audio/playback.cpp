#include "playback.h"

#include <dpp/discordclient.h>
#include <dpp/discordvoiceclient.h>
#include <spdlog/spdlog.h>
#include <chrono>
#include <thread>

namespace {
constexpr int MIN_REMAINING_SECONDS = 5;
} // namespace

void stream_to_voice(decoder& dec, dpp::discord_voice_client* voice_client, const std::stop_token& token) {
    std::vector<int16_t> pcm;
    std::vector<int16_t> buf;
    while (!token.stop_requested()) {
        while (voice_client->get_secs_remaining() > MIN_REMAINING_SECONDS && !token.stop_requested()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
        auto result = dec.read_pcm(pcm);
        if (result == read_result::error) {
            spdlog::error("Failed to read pcm");
            break;
        }
        if (result == read_result::stopped) {
            break;
        }
        if (result == read_result::data) {
            buf.insert(buf.end(), pcm.begin(), pcm.end());
        }
        if (result == read_result::end_of_stream) {
            if (!buf.empty()) {
                buf.resize(dpp::send_audio_raw_max_length / sizeof(int16_t));
                voice_client->send_audio_raw(
                        reinterpret_cast<uint16_t*>(buf.data()), dpp::send_audio_raw_max_length);
                buf.clear();
            }
            break;
        }
        auto bytes = buf.size() * sizeof(int16_t);
        while (bytes >= dpp::send_audio_raw_max_length) {
            voice_client->send_audio_raw(
                    reinterpret_cast<uint16_t*>(buf.data()), dpp::send_audio_raw_max_length);
            buf.erase(buf.begin(), buf.begin() + dpp::send_audio_raw_max_length / sizeof(int16_t));
            bytes -= dpp::send_audio_raw_max_length;
        }
    }
}