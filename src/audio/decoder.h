#pragma once
#include <filesystem>
#include <vector>

struct AVFormatContext;
struct AVCodecContext;
struct AVCodec;
struct AVPacket;
struct AVFrame;
struct SwrContext;

enum class read_result { data, error, end_of_stream };

void print_version();

class decoder {
public:
    decoder() = default;

    decoder(const decoder&) = delete;

    decoder& operator=(const decoder&) = delete;

    ~decoder();

    int open(const std::filesystem::path& file_path);

    read_result read_pcm(std::vector<int16_t>& pcm);

private:
    AVCodecContext* codec_context = nullptr;
    AVFormatContext* format_context = nullptr;
    const AVCodec* codec = nullptr;
    AVFrame* frame = nullptr;
    AVPacket* packet = nullptr;
    SwrContext* swr_context = nullptr;
    int audio_stream_ind = -1;

    enum class states { reading, draining, finished };

    states state = states::reading;
};
