//
// Created by adebayo on 4/25/21.
//

#ifndef LIBMEDIAFYANDROID_ENCODER_H
#define LIBMEDIAFYANDROID_ENCODER_H

#include <vector>

extern "C" {
#include <libavcodec/avcodec.h>
};

#include <atomic>
#include <mutex>
#include "Deleters.h"
#include <memory>

namespace libmediafy {
    class Encoder {
    public:
        Encoder(AVCodecID codec_id);
        virtual bool initialize(AVCodecParameters* params);
        void set_time_base(AVRational tb);
        void release();
        bool is_initialized() {
            return initialized;
        }
        int get_frame_size() { return codec_ctx->frame_size; }
        std::vector<std::shared_ptr<AVPacket>> encode_frame(AVFrame* frame, int* res);
        std::vector<std::shared_ptr<AVPacket>> flush(int* res);
        void set_global_header() {
            codec_ctx->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
        }

    protected:
        std::vector<std::shared_ptr<AVPacket>> encode_frame_internal(AVFrame* frame, int* res);
        std::atomic_bool initialized{false};
        std::atomic_bool flushed{false};
        AVCodec* encoder{nullptr};
        AVCodecContext* codec_ctx{nullptr};
        std::mutex lock{};
    };
}
#endif //LIBMEDIAFYANDROID_ENCODER_H
