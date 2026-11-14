/*
process_tool — runs an arbitrary command via process::run and prints
its exit code and stdout. Used to test the process module.

Usage:
  process_tool <program> [args...]

Examples:
  process_tool echo hello
  process_tool false
  process_tool yt-dlp --version
*/
#include "process/process.h"

#include <iostream>
#include <spdlog/spdlog.h>


int main(int argc, char** argv) {
    std::vector<std::string> args(argv + 1, argv + argc);
    auto res = process::run(args, std::chrono::seconds{10});
    if (!res) {
        spdlog::error("Failed to run process");
        return 1;
    }
    int code = res->code;
    std::cout << std::format("exit code: {}\nstdout_data: {}\nstderr_data: {}\ntimeout: {}", code, res->stdout_data,
            res->stderr_data, res->is_timeout);
}