#include "decoder.h"

#include <spdlog/spdlog.h>

extern "C" {
#include <libavutil/avutil.h>
#include <libavutil/error.h>
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
}


void print_version() {
    auto version = av_version_info();
    spdlog::info("FFmpeg version: {}", version);
}

static std::string av_err_to_string(int err) {
    char errbuf[AV_ERROR_MAX_STRING_SIZE];

    av_strerror(err, errbuf, sizeof(errbuf));

    return std::string(errbuf);
}

decoder::~decoder() {
    avcodec_free_context(&codec_context);
    avformat_close_input(&format_context);
    av_frame_free(&frame);
    av_packet_free(&packet);
}

int decoder::open(const std::filesystem::path& file_path) {
    int err_code = avformat_open_input(&format_context, file_path.c_str(), nullptr, nullptr);
    if (err_code < 0) {
        spdlog::error("Failed to open input file {}", av_err_to_string(err_code));
        return err_code;
    }
    err_code = avformat_find_stream_info(format_context, nullptr);
    if (err_code < 0) {
        spdlog::error("Failed to read stream info: {}", av_err_to_string(err_code));
        return err_code;
    }
    audio_stream_ind = av_find_best_stream(format_context, AVMEDIA_TYPE_AUDIO, -1, -1, &codec, 0);
    if (audio_stream_ind < 0) {
        spdlog::error("No audio stream found: {}", av_err_to_string(audio_stream_ind));
        return audio_stream_ind;
    }
    codec_context = avcodec_alloc_context3(codec);
    if (!codec_context) {
        spdlog::error("Failed to allocate codec context");
        return AVERROR(ENOMEM);
    }
    err_code = avcodec_parameters_to_context(codec_context, format_context->streams[audio_stream_ind]->codecpar);
    if (err_code < 0) {
        spdlog::error("Failed to copy codec parameters: {}", av_err_to_string(err_code));
        return err_code;
    }
    err_code = avcodec_open2(codec_context, codec, nullptr);
    if (err_code < 0) {
        spdlog::error("Failed to open codec: {}", av_err_to_string(err_code));
        return err_code;
    }
    spdlog::info("Audio stream: codec={}, sample_rate={} Hz, channels={}, sample_format={}", codec->name,
            codec_context->sample_rate, codec_context->ch_layout.nb_channels,
            av_get_sample_fmt_name(codec_context->sample_fmt));
    spdlog::info("{}", format_context->duration);

    return 0;
}