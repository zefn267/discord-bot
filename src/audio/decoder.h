#pragma once

#include <string>
#include <vector>
#include <memory>
#include <stop_token>
#include <unordered_map>

struct AVFormatContext;
struct AVCodecContext;
struct AVCodec;
struct AVPacket;
struct AVFrame;
struct SwrContext;

struct av_deleter {
    void operator()(AVFormatContext* format_context) const;
    void operator()(AVCodecContext* codec_context) const;
    void operator()(SwrContext* swr_context) const;
    void operator()(AVFrame* frame) const;
    void operator()(AVPacket* packet) const;
};

using avformat_context_ptr = std::unique_ptr<AVFormatContext, av_deleter>;
using avcodec_context_ptr = std::unique_ptr<AVCodecContext, av_deleter>;
using av_packet_ptr = std::unique_ptr<AVPacket, av_deleter>;
using av_frame_ptr = std::unique_ptr<AVFrame, av_deleter>;
using swr_context_ptr = std::unique_ptr<SwrContext, av_deleter>;

enum class read_result { data, error, end_of_stream, stopped};

void init_ffmpeg();

class decoder {
public:
    int open(const std::string& url, const std::unordered_map<std::string, std::string>& headers = {});

    read_result read_pcm(std::vector<int16_t>& pcm);

    void set_stop_token(std::stop_token token);

private:
    std::unique_ptr<std::stop_token> stop_token_ = std::make_unique<std::stop_token>();
    avcodec_context_ptr codec_context_;
    avformat_context_ptr format_context_;
    const AVCodec* codec_ = nullptr;
    av_frame_ptr frame_;
    av_packet_ptr packet_;
    swr_context_ptr swr_context_;
    int audio_stream_ind_ = -1;

    enum class states { reading, draining, finished };

    states state = states::reading;
};
