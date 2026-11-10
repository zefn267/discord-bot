/*
stream_tool — fetches a track by URL via yt-dlp, opens the audio stream
over the network and decodes it into raw PCM (s16le, 48 kHz, stereo).

Usage:
  stream_tool <url> <output.raw>

Example:
  stream_tool "https://www.youtube.com/watch?v=..." out.raw
  aplay -f S16_LE -r 48000 -c 2 out.raw
*/
#include "audio/decoder.h"
#include "ytdlp/soundtrack_info.h"

#include <fstream>

int main(int argc, char **argv) {
    std::string url = argv[1];
    auto info = ytdlp::fetch_track_info(url);
    init_ffmpeg();
    if (!info) {
        return 1;
    }
    decoder dec;
    if (dec.open(info->stream_url, info->http_headers) < 0) {
        return 1;
    }
    std::ofstream file(argv[2], std::ios::binary);
    if (!file.is_open()) {
        return 1;
    }
    std::vector<int16_t> pcm;
    while (true) {
        auto result = dec.read_pcm(pcm);
        if (result == read_result::data) {
            file.write(reinterpret_cast<const char*>(pcm.data()), pcm.size() * sizeof(int16_t));
        }
        if (result == read_result::end_of_stream) {
            break;
        }
        if (result == read_result::error) {
            return 1;
        }
    }
    file.close();
}
