#include "playback.h"

#include <dpp/discordclient.h>
#include <dpp/discordvoiceclient.h>
#include <spdlog/spdlog.h>
#include <chrono>
#include <thread>

namespace {
constexpr int TARGET_BUFFER_SECONDS = 5;
} // namespace

void stream_to_voice(
        decoder& dec, const std::function<dpp::discord_voice_client*()>& get_client, const std::stop_token& token) {
    std::vector<int16_t> pcm;
    std::vector<int16_t> buf;
    while (!token.stop_requested()) {
        dpp::discord_voice_client* voice = nullptr;
        while (!token.stop_requested()) {
            voice = get_client();
            if (!voice) {
                spdlog::warn("Voice connection lost");
                return;
            }
            if (voice->get_secs_remaining() <= TARGET_BUFFER_SECONDS) {
                break;
            }
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
                voice = get_client();
                if (!voice) {
                    spdlog::warn("Voice connection lost");
                    return;
                }
                voice->send_audio_raw(reinterpret_cast<uint16_t*>(buf.data()), dpp::send_audio_raw_max_length);
                buf.clear();
            }
            break;
        }
        auto bytes = buf.size() * sizeof(int16_t);
        while (bytes >= dpp::send_audio_raw_max_length) {
            voice = get_client();
            if (!voice) {
                spdlog::warn("Voice connection lost");
                return;
            }
            voice->send_audio_raw(reinterpret_cast<uint16_t*>(buf.data()), dpp::send_audio_raw_max_length);
            buf.erase(buf.begin(), buf.begin() + dpp::send_audio_raw_max_length / sizeof(int16_t));
            bytes -= dpp::send_audio_raw_max_length;
        }
    }
}