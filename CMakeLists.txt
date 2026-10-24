cmake_minimum_required(VERSION 3.21)
project(discord_bot LANGUAGES CXX)

set(CMAKE_CXX_EXTENSIONS OFF)

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
)

target_include_directories(discord_bot_core PUBLIC src)
target_compile_features(discord_bot_core PUBLIC cxx_std_20)
target_compile_options(discord_bot_core PRIVATE -Wall -Wextra -Wpedantic)

target_link_libraries(discord_bot_core
        PUBLIC  dpp::dpp
        PRIVATE spdlog::spdlog
)

add_executable(discord_bot src/main.cpp)

target_compile_options(discord_bot PRIVATE -Wall -Wextra -Wpedantic)

target_include_directories(discord_bot SYSTEM PRIVATE ${FFMPEG_INCLUDE_DIRS})
target_link_directories(discord_bot PRIVATE ${FFMPEG_LIBRARY_DIRS})

target_link_libraries(discord_bot PRIVATE
        discord_bot_core
        spdlog::spdlog
        ${FFMPEG_LIBRARIES}
)