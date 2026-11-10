/*
decode_tool — decodes a local audio file into raw PCM (s16le, 48 kHz, stereo).

Usage:
  decode_tool <input_file> <output.raw>

Example:
  decode_tool test_data/input.mp3 out.raw
  aplay -f S16_LE -r 48000 -c 2 out.raw
*/
#include "audio/decoder.h"
#include "logging.h"

#include <fstream>
#include <spdlog/spdlog.h>


int main(int argc, char** argv) {
    set_settings_logs();

    init_ffmpeg();
    if (argc < 3) {
        spdlog::error("Usage: {} <input_file> <output_file>", argv[0]);
        return 1;
    }
    decoder dec;
    if (dec.open(argv[1]) < 0) {
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
