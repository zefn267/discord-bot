#include "decoder.h"

#include <spdlog/spdlog.h>
#include <format>

extern "C" {
#include <libavutil/avutil.h>
#include <libavutil/error.h>
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libswresample/swresample.h>
#include <libavutil/channel_layout.h>
#include <libavutil/dict.h>
}

namespace {
constexpr AVSampleFormat OUTPUT_FORMAT = AV_SAMPLE_FMT_S16;
constexpr int OUTPUT_SAMPLE_RATE = 48000;
constexpr int OUTPUT_CHANNELS = 2;

class av_dict {
public:
    av_dict() = default;

    ~av_dict() { av_dict_free(&av_dict_); }

    av_dict(const av_dict&) = delete;

    av_dict& operator=(const av_dict&) = delete;

    AVDictionary** get_address() { return &av_dict_; }

private:
    AVDictionary* av_dict_ = nullptr;
};

std::string av_err_to_string(int err) {
    char errbuf[AV_ERROR_MAX_STRING_SIZE];

    av_strerror(err, errbuf, sizeof(errbuf));

    return errbuf;
}

int interrupt_cb(void* opaque) {
    auto* token = static_cast<std::stop_token*>(opaque);
    return (token && token->stop_requested()) ? 1 : 0;
}

void prepare_output_frame(AVFrame& frame) {
    frame.format = OUTPUT_FORMAT;
    frame.sample_rate = OUTPUT_SAMPLE_RATE;
    av_channel_layout_default(&frame.ch_layout, OUTPUT_CHANNELS);
}

void append_samples(std::vector<int16_t>& pcm, const AVFrame& frame) {
    auto c = frame.nb_samples * OUTPUT_CHANNELS;
    auto ptr = reinterpret_cast<const int16_t*>(frame.data[0]);
    pcm.insert(pcm.end(), ptr, ptr + c);
}
} // namespace
void init_ffmpeg() {
    auto version = av_version_info();
    av_log_set_level(AV_LOG_WARNING);
    spdlog::info("FFmpeg version: {}", version);
}

void av_deleter::operator()(AVCodecContext* codec_context) const {
    avcodec_free_context(&codec_context);
}

void av_deleter::operator()(AVFrame* frame) const {
    av_frame_free(&frame);
}

void av_deleter::operator()(AVPacket* packet) const {
    av_packet_free(&packet);
}

void av_deleter::operator()(SwrContext* swr_context) const {
    swr_free(&swr_context);
}

void av_deleter::operator()(AVFormatContext* format_context) const {
    avformat_close_input(&format_context);
}

void decoder::set_stop_token(std::stop_token token) {
    *stop_token_ = std::move(token);
}

int decoder::open(const std::string& url, const std::unordered_map<std::string, std::string>& headers) {
    AVFormatContext* fmt_context = avformat_alloc_context();
    if (!fmt_context) {
        spdlog::error("Failed to allocate format context");
        return AVERROR(ENOMEM);
    }
    fmt_context->interrupt_callback.callback = interrupt_cb;
    fmt_context->interrupt_callback.opaque = stop_token_.get();
    av_dict dict;
    if (!headers.empty()) {
        std::string all_headers;
        for (const auto& [header, value] : headers) {
            all_headers += std::format("{}: {}\r\n", header, value);
        }
        av_dict_set(dict.get_address(), "headers", all_headers.c_str(), 0);
    }
    if (url.starts_with("https://") || url.starts_with("http://")) {
        av_dict_set(dict.get_address(), "rw_timeout", "10000000", 0);
        av_dict_set(dict.get_address(), "reconnect", "1", 0);
        av_dict_set(dict.get_address(), "reconnect_streamed", "1", 0);
        av_dict_set(dict.get_address(), "reconnect_on_network_error", "1", 0);
        av_dict_set(dict.get_address(), "reconnect_on_http_error", "500,502,503,504", 0);
        av_dict_set(dict.get_address(), "reconnect_delay_max", "5", 0);
    }
    int code = avformat_open_input(&fmt_context, url.c_str(), nullptr, dict.get_address());
    if (code < 0) {
        spdlog::error("Failed to open input file '{}': {}", url.c_str(), av_err_to_string(code));
        return code;
    }
    format_context_.reset(fmt_context);
    code = avformat_find_stream_info(format_context_.get(), nullptr);
    if (code < 0) {
        spdlog::error("Failed to read stream info: {}", av_err_to_string(code));
        return code;
    }
    audio_stream_ind_ = av_find_best_stream(format_context_.get(), AVMEDIA_TYPE_AUDIO, -1, -1, &codec_, 0);
    if (audio_stream_ind_ < 0) {
        spdlog::error("No audio stream found: {}", av_err_to_string(audio_stream_ind_));
        return audio_stream_ind_;
    }
    codec_context_.reset(avcodec_alloc_context3(codec_));
    if (!codec_context_) {
        spdlog::error("Failed to allocate codec context");
        return AVERROR(ENOMEM);
    }
    code = avcodec_parameters_to_context(codec_context_.get(), format_context_->streams[audio_stream_ind_]->codecpar);
    if (code < 0) {
        spdlog::error("Failed to copy codec parameters: {}", av_err_to_string(code));
        return code;
    }
    code = avcodec_open2(codec_context_.get(), codec_, nullptr);
    if (code < 0) {
        spdlog::error("Failed to open codec: {}", av_err_to_string(code));
        return code;
    }
    spdlog::info("Audio stream: codec={}, sample_rate={} Hz, channels={}, sample_format={}", codec_->name,
            codec_context_->sample_rate, codec_context_->ch_layout.nb_channels,
            av_get_sample_fmt_name(codec_context_->sample_fmt));
    spdlog::info("Duration: {:.3f} sec", static_cast<double>(format_context_->duration) / AV_TIME_BASE);
    packet_.reset(av_packet_alloc());
    if (!packet_) {
        spdlog::error("Failed to allocate packet");
        return AVERROR(ENOMEM);
    }
    frame_.reset(av_frame_alloc());
    if (!frame_) {
        spdlog::error("Failed to allocate frame");
        return AVERROR(ENOMEM);
    }
    out_frame_.reset(av_frame_alloc());
    if (!out_frame_) {
        spdlog::error("Failed to allocate out_frame");
        return AVERROR(ENOMEM);
    }
    auto swr = swr_alloc();
    if (!swr) {
        spdlog::error("Failed to allocate swr context");
        return AVERROR(ENOMEM);
    }
    swr_context_.reset(swr);

    return 0;
}

read_result decoder::read_pcm(std::vector<int16_t>& pcm) {
    pcm.clear();
    while (true) {
        int code = avcodec_receive_frame(codec_context_.get(), frame_.get());
        if (code == 0) {
            prepare_output_frame(*out_frame_);
            int res = swr_convert_frame(swr_context_.get(), out_frame_.get(), frame_.get());
            if (res == AVERROR_INPUT_CHANGED) {
                auto r = swr_convert_frame(swr_context_.get(), out_frame_.get(), nullptr);
                if (r >= 0) {
                    append_samples(pcm, *out_frame_);
                }
                av_frame_unref(out_frame_.get());
                prepare_output_frame(*out_frame_);
                auto code_result = swr_config_frame(swr_context_.get(), out_frame_.get(), frame_.get());
                if (code_result < 0) {
                    spdlog::error("Failed to config_frame: {}", av_err_to_string(code_result));
                    return read_result::error;
                }
                code_result = swr_init(swr_context_.get());
                if (code_result < 0) {
                    spdlog::error("Failed to init swr: {}", av_err_to_string(code_result));
                    return read_result::error;
                }
                spdlog::info("Audio format changed: {}, {} HZ", frame_->format, frame_->sample_rate);
                res = swr_convert_frame(swr_context_.get(), out_frame_.get(), frame_.get());
            }
            av_frame_unref(frame_.get());
            if (res < 0) {
                av_frame_unref(out_frame_.get());
                spdlog::error("Failed to convert frame: {}", av_err_to_string(res));
                return read_result::error;
            }
            append_samples(pcm, *out_frame_);
            av_frame_unref(out_frame_.get());
            if (!pcm.empty()) {
                return read_result::data;
            }
        } else if (code == AVERROR(EAGAIN)) {
            int result = av_read_frame(format_context_.get(), packet_.get());

            if (result == AVERROR_EOF) {
                int result = avcodec_send_packet(codec_context_.get(), nullptr);
                if (result < 0) {
                    spdlog::error("Failed to send packet with nullptr: {}", av_err_to_string(result));
                    return read_result::error;
                }
                state = states::draining;
                continue;
            }
            if (result == AVERROR_EXIT) {
                return read_result::stopped;
            }
            if (result < 0) {
                spdlog::error("Failed to read packet: {}", av_err_to_string(result));
                return read_result::error;
            }
            if (packet_->stream_index != audio_stream_ind_) {
                av_packet_unref(packet_.get());
                continue;
            }
            result = avcodec_send_packet(codec_context_.get(), packet_.get());
            av_packet_unref(packet_.get());
            if (result < 0) {
                spdlog::error("Failed to send packet: {}", av_err_to_string(result));
                return read_result::error;
            }
        } else if (code == AVERROR_EOF) {
            if (state == states::finished) {
                return read_result::end_of_stream;
            }
            if (!swr_is_initialized(swr_context_.get())) {
                return read_result::end_of_stream;
            }
            prepare_output_frame(*out_frame_);
            auto res = swr_convert_frame(swr_context_.get(), out_frame_.get(), nullptr);
            if (res < 0) {
                av_frame_unref(out_frame_.get());
                spdlog::error("Failed to convert frame: {}", av_err_to_string(res));
                return read_result::error;
            }
            append_samples(pcm, *out_frame_);
            av_frame_unref(out_frame_.get());
            state = states::finished;
            if (!pcm.empty()) {
                return read_result::data;
            }
            return read_result::end_of_stream;
        } else {
            spdlog::error("An error occurred: {}", av_err_to_string(code));
            return read_result::error;
        }
    }
}
