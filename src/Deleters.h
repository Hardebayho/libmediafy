//
// Created by adebayo on 5/7/21.
//

#ifndef LIBMEDIAFYANDROID_DELETERS_H
#define LIBMEDIAFYANDROID_DELETERS_H

extern "C" {
#include <libavutil/frame.h>
#include <libavformat/avformat.h>
};

namespace libmediafy {
    struct PacketDeleter {
        void operator()(AVPacket* packet) {
            av_packet_free(&packet);
        }
    };

    struct FrameDeleter {
        void operator()(AVFrame* frame) {
            av_frame_free(&frame);
        }
    };

    struct AVCodecParamsDeleter {
        void operator()(AVCodecParameters* params) {
            avcodec_parameters_free(&params);
        }
    };

    struct FormatCtxDeleter {
        void operator()(AVFormatContext*& ctx) {
            if (ctx->iformat) avformat_close_input(&ctx);
            else avformat_free_context(ctx);
        }
    };
}

#endif //LIBMEDIAFYANDROID_DELETERS_H
