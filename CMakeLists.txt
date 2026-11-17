cmake_minimum_required(VERSION 3.21)
project(discord_bot LANGUAGES CXX)

set(CMAKE_CXX_EXTENSIONS OFF)

set(PROJECT_WARNINGS -Wall -Wextra -Wpedantic -Wshadow)

find_package(dpp CONFIG REQUIRED)
find_package(spdlog CONFIG REQUIRED)
find_package(FFMPEG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)

add_library(discord_bot_core STATIC
        src/bot.cpp
        src/bot.h
        src/config.cpp
        src/config.h
        src/logging.cpp
        src/logging.h

        src/commands/command.h
        src/commands/command_context.h
        src/commands/command_registry.cpp
        src/commands/command_registry.h
        src/commands/general_commands.cpp
        src/commands/general_commands.h
        src/commands/voice_commands.cpp
        src/commands/voice_commands.h
        src/commands/music_commands.h
        src/commands/music_commands.cpp

        src/voice/voice_checks.cpp
        src/voice/voice_checks.h

        src/audio/decoder.cpp
        src/audio/decoder.h
        src/audio/playback.h
        src/audio/playback.cpp

        src/process/process.cpp
        src/process/process.h
        src/process/unique_fd.cpp
        src/process/unique_fd.h

        src/ytdlp/soundtrack_info.cpp
        src/ytdlp/soundtrack_info.h

        src/player/track_queue.h
        src/player/track_queue.cpp
        src/player/guild_player.h
        src/player/guild_player.cpp
        src/player/player_manager.h
        src/player/player_manager.cpp
)

target_include_directories(discord_bot_core PUBLIC src)
target_compile_features(discord_bot_core PUBLIC cxx_std_20)
target_compile_options(discord_bot_core PRIVATE ${PROJECT_WARNINGS})

target_precompile_headers(discord_bot_core PRIVATE <dpp/dpp.h> <spdlog/spdlog.h>)

target_include_directories(discord_bot_core SYSTEM PRIVATE ${FFMPEG_INCLUDE_DIRS})
target_link_directories(discord_bot_core PUBLIC ${FFMPEG_LIBRARY_DIRS})

target_link_libraries(discord_bot_core
        PUBLIC  dpp::dpp
        PRIVATE spdlog::spdlog
        ${FFMPEG_LIBRARIES}
)

add_executable(discord_bot src/main.cpp)
target_compile_options(discord_bot PRIVATE ${PROJECT_WARNINGS})
target_link_libraries(discord_bot PRIVATE discord_bot_core spdlog::spdlog)

add_executable(decode_tool tools/decode_tool.cpp)
target_compile_options(decode_tool PRIVATE ${PROJECT_WARNINGS})
target_link_libraries(decode_tool PRIVATE discord_bot_core spdlog::spdlog)

add_executable(process_tool tools/process_tool.cpp)
target_compile_options(process_tool PRIVATE ${PROJECT_WARNINGS})
target_link_libraries(process_tool PRIVATE discord_bot_core spdlog::spdlog)

add_executable(ytdlp_tool tools/ytdlp_tool.cpp)
target_compile_options(ytdlp_tool PRIVATE ${PROJECT_WARNINGS})
target_link_libraries(ytdlp_tool PRIVATE discord_bot_core spdlog::spdlog nlohmann_json::nlohmann_json)

add_executable(stream_tool tools/stream_tool.cpp)
target_compile_options(stream_tool PRIVATE ${PROJECT_WARNINGS})
target_link_libraries(stream_tool PRIVATE discord_bot_core spdlog::spdlog )

enable_testing()
find_package(GTest CONFIG REQUIRED)

add_executable(unit_tests
        tests/track_queue_test.cpp
)
target_link_libraries(unit_tests PRIVATE discord_bot_core GTest::gtest_main)
target_compile_options(unit_tests PRIVATE ${PROJECT_WARNINGS})

include(GoogleTest)
gtest_discover_tests(unit_tests)