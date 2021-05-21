//
// Created by adebayo on 4/25/21.
//

#include "Encoder.h"

libmediafy::Encoder::Encoder(AVCodecID codec_id) {
    encoder = avcodec_find_encoder(codec_id);
    if (encoder) codec_ctx = avcodec_alloc_context3(encoder);
    initialized = false;
}

void libmediafy::Encoder::release() {
    if (codec_ctx) {
        avcodec_free_context(&codec_ctx);
        encoder = nullptr;
    }
}

std::vector<std::shared_ptr<AVPacket>> libmediafy::Encoder::encode_frame(AVFrame *frame, int* res) {
    std::unique_lock<std::mutex> lock1{lock};
    return encode_frame_internal(frame, res);
}

std::vector<std::shared_ptr<AVPacket>> libmediafy::Encoder::encode_frame_internal(AVFrame *frame, int *res) {
    auto packets = std::vector<std::shared_ptr<AVPacket>>();

    if (!initialized || flushed) {
        return packets;
    }

    *res = avcodec_send_frame(codec_ctx, frame);
    AVPacket *packet = av_packet_alloc();
    while (!(*res = avcodec_receive_packet(codec_ctx, packet))) {
        packets.emplace_back(std::shared_ptr<AVPacket>(packet, PacketDeleter()));
        packet = av_packet_alloc();
        AVSampleFormat fmt;
    }
    av_packet_free(&packet);
    return packets;
}

std::vector<std::shared_ptr<AVPacket>> libmediafy::Encoder::flush(int* res) {
    std::unique_lock<std::mutex> lock1{lock};
    flushed = true;
    return encode_frame_internal(nullptr, res);
}

bool libmediafy::Encoder::initialize(AVCodecParameters* params) {
    std::unique_lock<std::mutex> lock1{lock};
    if (initialized) return true;
    if (!codec_ctx || !params) return false;

    avcodec_parameters_to_context(codec_ctx, params);

    int ret = avcodec_open2(codec_ctx, encoder, nullptr);
    if (ret < 0) {
        char errorLog[512];
        fprintf(stderr, "Encoder couldn't be configured: %s\n", av_make_error_string(errorLog, 512, ret));
    }
    initialized = ret == 0;
    return ret == 0;
}

void libmediafy::Encoder::set_time_base(AVRational tb) {
    std::unique_lock<std::mutex> lock1{lock};
    if (codec_ctx) {
        codec_ctx->time_base = tb;
    }
}
