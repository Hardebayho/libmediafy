//
// Created by adebayo on 4/3/21.
//

#include "Media.h"
#include "Deleters.h"

std::shared_ptr<libmediafy::Media> libmediafy::MediaBuilder::create() {
    if (url.empty()) return nullptr;
    AVFormatContext* ctx{nullptr};
    std::shared_ptr<AVFormatContext> format_ctx{nullptr};
    int ret = 0;
    if (muxer) {
        if (descriptions.empty()) {
            return nullptr;
        }

        ret = avformat_alloc_output_context2(&ctx, fmt, nullptr, url.c_str());
        if (ret < 0) {
            fprintf(stderr, "Ret: %d\n", ret);
            return nullptr;
        }
        format_ctx = std::shared_ptr<AVFormatContext>(ctx, FormatCtxDeleter());
        for (auto& desc : descriptions) {
            auto& description = desc.description;
            auto& metadata = desc.metadata;
            auto* stream = avformat_new_stream(ctx, nullptr);
            if (!stream) {
                return nullptr;
            }
            ret = avcodec_parameters_copy(stream->codecpar, description.get());
            if (ret < 0) {
                return nullptr;
            }
            if (metadata) {
                ret = av_dict_copy(&stream->metadata, metadata.get(), 0);
                if (ret < 0) {
                    return nullptr;
                }
            }
        }
        if (!(ctx->flags & AVFMT_NOFILE)) {
            ret = avio_open(&ctx->pb, url.c_str(), AVIO_FLAG_WRITE);
            if (ret < 0) {
                fprintf(stderr, "unable to open the OUPUT FILE! URL: %s\n", url.c_str());
                return nullptr;
            }
        }

        ctx->flags &= flags;

        ret = avformat_write_header(ctx, nullptr);
        if (ret < 0) {
            char infoLog[512];
            fprintf(stderr, "Unable to write header! Error: %s\n", av_make_error_string(infoLog, 512, ret));
            return nullptr;
        }
    } else {
        ret = avformat_open_input(&ctx, url.c_str(), nullptr, nullptr);
        if (ret < 0) {
            char buf[512];
            printf("Couldn't open input file. Error: %s", av_make_error_string(buf, 512, ret));
            return nullptr;
        }
        format_ctx = std::shared_ptr<AVFormatContext>(ctx, FormatCtxDeleter());
        ret = avformat_find_stream_info(ctx, nullptr);
        ctx->flags &= flags;
        if (ret < 0 || ctx->nb_streams <= 0) {
            return nullptr;
        }
    }

    auto media = std::shared_ptr<libmediafy::Media>(new(std::nothrow) Media());
    if (!media) return nullptr;
    media->muxer = muxer;
    media->format_ctx = format_ctx;
    return media;
}

namespace libmediafy {
    std::shared_ptr<AVPacket> Media::read() {
        std::unique_lock<std::mutex> lock{media_lock};
        if (muxer) return nullptr;
        AVPacket* packet = av_packet_alloc();
        if (!packet) return nullptr;

        int ret = av_read_frame(format_ctx.get(), packet);
        if (ret < 0) {
            av_packet_free(&packet);
            return nullptr;
        }

        if (packet->pts != AV_NOPTS_VALUE) packet->pts = static_cast<int64_t>(packet->pts * 1000 *
                                           av_q2d(format_ctx->streams[packet->stream_index]->time_base));
        if (packet->dts != AV_NOPTS_VALUE) packet->dts = static_cast<int64_t>(packet->dts * 1000 *
                                           av_q2d(format_ctx->streams[packet->stream_index]->time_base));
        return std::shared_ptr<AVPacket>(packet);
    }

    AVPacket* Media::get_album_art() {
        std::unique_lock<std::mutex> lock{media_lock};
        if (muxer) return nullptr;
        for (int i = 0; i < format_ctx->nb_streams; i++) {
            if (format_ctx->streams[i]->disposition & AV_DISPOSITION_ATTACHED_PIC) {
                return av_packet_clone(&format_ctx->streams[i]->attached_pic);
            }
        }

        return nullptr;
    }

    bool Media::seek(int64_t position) {
        std::unique_lock<std::mutex> lock{media_lock};
        if (muxer || position < 0 || position >= format_ctx->duration) return false;
        return av_seek_frame(format_ctx.get(), -1, position * 1000, 0) == 0;
    }

    int64_t Media::duration() {
        std::unique_lock<std::mutex> lock{media_lock};
        if (muxer) return 0;
        return format_ctx->duration / 1000;
    }

    bool Media::write(AVPacket* packet) {
        std::unique_lock<std::mutex> lock{media_lock};
        if (!muxer || !packet || packet->stream_index < 0 || packet->stream_index >= format_ctx->nb_streams) {
            return false;
        }
        if (packet->pts != AV_NOPTS_VALUE) {
            double time = packet->pts / 1000.0;
            packet->pts = time * (format_ctx->streams[packet->stream_index]->time_base.den);
        }
        if (packet->dts != AV_NOPTS_VALUE) {
            double time = packet->dts / 1000.0;
            packet->dts = time * (format_ctx->streams[packet->stream_index]->time_base.den);
        }
        int ret = av_interleaved_write_frame(format_ctx.get(), packet);
        if (ret < 0) {
            char infoLog[512];
            fprintf(stderr, "Writing packet resulted in: %s\n", av_make_error_string(infoLog, 512, ret));
        }
        return ret == 0;
    }

    bool Media::requires_global_header() {
        std::unique_lock<std::mutex> lock{media_lock};
        if (!muxer) return false;
        return static_cast<bool>(format_ctx->flags & AVFMT_GLOBALHEADER);
    }

    bool Media::flush() {
        std::unique_lock<std::mutex> lock{media_lock};
        if (!muxer) return false;
        return av_interleaved_write_frame(format_ctx.get(), nullptr) == 0;
    }

    int Media::nb_streams() {
        std::unique_lock<std::mutex> lock{media_lock};
        return format_ctx->nb_streams;
    }

    const AVStream *const Media::stream(int index) {
        std::unique_lock<std::mutex> lock{media_lock};
        if (index >= format_ctx->nb_streams) return nullptr;
        return format_ctx->streams[index];
    }

    const AVInputFormat* Media::iformat() {
        return format_ctx->iformat;
    }
    const AVOutputFormat* Media::oformat() {
        return format_ctx->oformat;
    }

    Media::~Media() {
        if (muxer) {
            int ret = av_write_trailer(format_ctx.get());
            if (ret < 0) printf("Couldn't write trailer!");
            else fprintf(stderr, "Wrote trailer!\n");
        }
    }
}

