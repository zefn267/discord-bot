#pragma once
#include <filesystem>


struct AVFormatContext;
struct AVCodecContext;
struct AVCodec;
struct AVPacket;
struct AVFrame;

void print_version();

class decoder {
public:
    decoder() = default;

    decoder(const decoder&) = delete;

    decoder& operator=(const decoder&) = delete;

    ~decoder();

    int open(const std::filesystem::path& file_path);
private:
    AVCodecContext* codec_context = nullptr;
    AVFormatContext* format_context  = nullptr;
    const AVCodec* codec = nullptr;
    AVFrame* frame = nullptr;
    AVPacket* packet = nullptr;
    int audio_stream_ind = -1;
};
