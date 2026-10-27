cmake_minimum_required(VERSION 3.21)
project(discord_bot LANGUAGES CXX)

set(CMAKE_CXX_EXTENSIONS OFF)

set(PROJECT_WARNINGS -Wall -Wextra -Wpedantic)

find_package(dpp CONFIG REQUIRED)
find_package(spdlog CONFIG REQUIRED)
find_package(FFMPEG REQUIRED)

add_library(discord_bot_core STATIC
        src/logging.cpp
        src/logging.h
        src/config.cpp
        src/config.h
        src/bot.cpp
        src/bot.h
        src/commands.h
        src/audio/decoder.cpp
        src/audio/decoder.h
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