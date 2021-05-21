//
// Created by adebayo on 4/2/21.
//

#ifndef LIBMEDIAFYANDROID_DECODER_H
#define LIBMEDIAFYANDROID_DECODER_H

#include <thread>
#include <mutex>
#include "concurrent_queue.h"
#include <atomic>
#include <vector>

extern "C" {
#include <libavcodec/avcodec.h>
};
#ifdef ANDROID
#include <jni.h>
#endif

namespace libmediafy {
    class Decoder {
    public:
        Decoder(AVCodecID codec_id, void* surface, bool ignore_hardware = false);
        ~Decoder();
        bool initialize(AVCodecParameters* params);
        void release();
        std::vector<std::shared_ptr<AVFrame>> decode_packet(AVPacket* packet, int* res);
        std::vector<std::shared_ptr<AVFrame>> flush();

        int get_output_format() { return output_format; }

    private:
        bool ignore_hardware;
        AVCodecID codec_id;
#ifdef ANDROID
        static JNIEnv* get_env();
#endif
        std::vector<std::shared_ptr<AVFrame>> decode_packet_internal(AVPacket* packet, int* res);
        AVCodec* decoder{nullptr};
        AVCodecContext* codec_ctx{nullptr};
        std::atomic_bool initialized{false};
        void create();
        AVCodecParameters* params{nullptr};
        void* vout_surface{nullptr};
        int output_format{-1};
        std::mutex lock{};
    };
}

#endif //LIBMEDIAFYANDROID_DECODER_H
