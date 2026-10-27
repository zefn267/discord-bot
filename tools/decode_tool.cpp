#include "audio/decoder.h"
#include "logging.h"

#include <fstream>
#include <spdlog/spdlog.h>


int main(int argc, char** argv) {
    set_settings_logs();

    print_version();
    if (argc < 3) {
        spdlog::error("Usage: {} <input_file> <output_file>", argv[0]);
        return 1;
    }
    decoder dec;
    if (dec.open(argv[1])) {
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