#include "audio/decoder.h"
#include "logging.h"

#include <spdlog/spdlog.h>


int main(int argc, char** argv) {
    set_settings_logs();

    print_version();
    decoder dec;
    dec.open(argv[1]);
    if (argc < 3) {
        spdlog::error("Usage: {} <input_file> <output_file>", argv[0]);
        return 1;
    }
}