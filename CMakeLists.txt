cmake_minimum_required(VERSION 3.21)
project(discord_bot LANGUAGES CXX)

find_package(dpp CONFIG REQUIRED)
find_package(spdlog CONFIG REQUIRED)
find_package(FFMPEG REQUIRED)

add_executable(discord_bot src/main.cpp)

target_compile_features(discord_bot PRIVATE cxx_std_20)
target_compile_options(discord_bot PRIVATE -Wall -Wextra -Wpedantic)

target_include_directories(discord_bot SYSTEM PRIVATE ${FFMPEG_INCLUDE_DIRS})
target_link_directories(discord_bot PRIVATE ${FFMPEG_LIBRARY_DIRS})

target_link_libraries(discord_bot PRIVATE
        dpp::dpp
        spdlog::spdlog
        ${FFMPEG_LIBRARIES}
)