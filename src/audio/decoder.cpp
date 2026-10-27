#include "decoder.h"

#include <spdlog/spdlog.h>

extern "C" {
#include <libavutil/avutil.h>
#include <libavutil/error.h>
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libswresample/swresample.h>
#include <libavutil/opt.h>
#include <libavutil/channel_layout.h>
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
    swr_free(&swr_context);
}

int decoder::open(const std::filesystem::path& file_path) {
    int code = avformat_open_input(&format_context, file_path.c_str(), nullptr, nullptr);
    if (code < 0) {
        spdlog::error("Failed to open input file '{}': {}", file_path.c_str(), av_err_to_string(code));
        return code;
    }
    code = avformat_find_stream_info(format_context, nullptr);
    if (code < 0) {
        spdlog::error("Failed to read stream info: {}", av_err_to_string(code));
        return code;
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
    code = avcodec_parameters_to_context(codec_context, format_context->streams[audio_stream_ind]->codecpar);
    if (code < 0) {
        spdlog::error("Failed to copy codec parameters: {}", av_err_to_string(code));
        return code;
    }
    code = avcodec_open2(codec_context, codec, nullptr);
    if (code < 0) {
        spdlog::error("Failed to open codec: {}", av_err_to_string(code));
        return code;
    }
    spdlog::info("Audio stream: codec={}, sample_rate={} Hz, channels={}, sample_format={}", codec->name,
            codec_context->sample_rate, codec_context->ch_layout.nb_channels,
            av_get_sample_fmt_name(codec_context->sample_fmt));
    spdlog::info("Duration: {:.3f} sec", static_cast<double>(format_context->duration) / AV_TIME_BASE);
    packet = av_packet_alloc();
    if (!packet) {
        spdlog::error("Failed to allocate packet");
        return AVERROR(ENOMEM);
    }
    frame = av_frame_alloc();
    if (!frame) {
        spdlog::error("Failed to allocate frame");
        return AVERROR(ENOMEM);
    }

    AVChannelLayout out_layout;
    av_channel_layout_default(&out_layout, 2);
    code = swr_alloc_set_opts2(&swr_context, &out_layout, AV_SAMPLE_FMT_S16, 48000, &codec_context->ch_layout,
            codec_context->sample_fmt, codec_context->sample_rate, 0, nullptr);
    if (code < 0) {
        spdlog::error("Failed to allocate swr context: {}", av_err_to_string(code));
        return code;
    }
    av_channel_layout_uninit(&out_layout);
    code = swr_init(swr_context);
    if (code < 0) {
        spdlog::error("Failed to initialize swr: {}", av_err_to_string(code));
        return code;
    }
    spdlog::info("Resampler initialized: {} Hz s16 ch{}", 48000, codec_context->ch_layout.nb_channels);

    return 0;
}

read_result decoder::read_pcm(std::vector<int16_t>& pcm) {
    while (true) {
        int code = avcodec_receive_frame(codec_context, frame);
        if (code == 0) {
            int max_out_samples = swr_get_out_samples(swr_context, frame->nb_samples);
            pcm.resize(max_out_samples * 2);
            auto ptr = reinterpret_cast<uint8_t*>(pcm.data());
            int gets = swr_convert(swr_context, &ptr, max_out_samples, frame->extended_data, frame->nb_samples);
            av_frame_unref(frame);
            if (gets < 0) {
                spdlog::error("Failed to resampling frame: {}", av_err_to_string(gets));
                return read_result::error;
            }
            pcm.resize(gets * 2);
            if (gets > 0) {
                return read_result::data;
            }
        } else if (code == AVERROR(EAGAIN)) {
            int result = av_read_frame(format_context, packet);

            if (result == AVERROR_EOF) {
                int result = avcodec_send_packet(codec_context, nullptr);
                if (result < 0) {
                    spdlog::error("Failed to send packet with nullptr: {}", av_err_to_string(result));
                    return read_result::error;
                }
                state = states::draining;
                continue;
            }
            if (result < 0) {
                spdlog::error("Failed to read packet: {}", av_err_to_string(result));
                return read_result::error;
            }
            if (packet->stream_index != audio_stream_ind) {
                av_packet_unref(packet);
                continue;
            }
            result = avcodec_send_packet(codec_context, packet);
            av_packet_unref(packet);
            if (result < 0) {
                spdlog::error("Failed to send packet: {}", av_err_to_string(result));
                return read_result::error;
            }
        } else if (code == AVERROR_EOF) {
            if (state == states::finished) {
                return read_result::end_of_stream;
            }
            int max_out_samples = swr_get_out_samples(swr_context, 0);
            pcm.resize(max_out_samples * 2);
            auto ptr = reinterpret_cast<uint8_t*>(pcm.data());
            int gets = swr_convert(swr_context, &ptr, max_out_samples, nullptr, 0);
            if (gets < 0) {
                spdlog::error("Failed to resampling frame: {}", av_err_to_string(gets));
                return read_result::error;
            }
            pcm.resize(gets * 2);
            state = states::finished;
            if (gets > 0) {
                return read_result::data;
            }
            return read_result::end_of_stream;
        } else {
            spdlog::error("An error occurred: {}", av_err_to_string(code));
            return read_result::error;
        }
    }
}
