//
// Created by adebayo on 4/2/21.
//

#include <vector>
#include "Decoder.h"
#include "Deleters.h"

extern "C" {
#include <libavcodec/mediacodec.h>
#include <libavutil/hwcontext_mediacodec.h>
#include <libavcodec/jni.h>
}

libmediafy::Decoder::Decoder(AVCodecID codec_id, void* surface, bool ignore_hardware) : codec_id(codec_id), ignore_hardware(ignore_hardware) {
#ifdef ANDROID
    if (surface) vout_surface = get_env()->NewGlobalRef((jobject)surface);
#endif
    create();
}

bool libmediafy::Decoder::initialize(AVCodecParameters *params) {
    if (initialized) return true;
    if (!decoder || !codec_ctx) {
        printf("No decoder|Codec Context for codec: %s\n", avcodec_get_name(params->codec_id));
        return false;
    }
    this->params = avcodec_parameters_alloc();
    avcodec_parameters_copy(this->params, params);
    int ret = avcodec_parameters_to_context(codec_ctx, params);
    if (ret < 0) return false;

    codec_ctx->thread_count = 2;
    ret = avcodec_open2(codec_ctx, decoder, nullptr);
    if (ret < 0) {
        printf("Couldn't open codec!");
        char res[512];
        printf("Error: %s", av_make_error_string(res, 512, ret));

        bool bad = false;
        if (output_format == AV_PIX_FMT_MEDIACODEC) {
            printf("Falling back to software implementation");
            output_format = AV_PIX_FMT_NONE;
            avcodec_free_context(&codec_ctx);
            AVCodecID id = decoder->id;
            decoder = avcodec_find_decoder(id);
            if (!decoder) {
                printf("No software implementation for this decoder!");
                bad = true;
            } else {
                codec_ctx = avcodec_alloc_context3(decoder);
                ret = avcodec_parameters_to_context(codec_ctx, params);
                if (ret < 0) {
                    bad = true;
                }
                codec_ctx->thread_count = 2;
                ret = avcodec_open2(codec_ctx, decoder, nullptr);
                if (ret < 0) {
                    printf("Couldn't open software codec too!");
                    bad = true;
                }
            }
        }
        if (bad) return false;
    }

    output_format = codec_ctx->codec_type == AVMEDIA_TYPE_AUDIO ? (int)codec_ctx->sample_fmt : (int)codec_ctx->pix_fmt;

    initialized = true;
    return true;
}

void libmediafy::Decoder::release() {
    if (!initialized) return;
    avcodec_free_context(&codec_ctx);
    initialized = false;
    output_format = AV_PIX_FMT_NONE;
    avcodec_parameters_free(&params);
}

std::vector<std::shared_ptr<AVFrame>> libmediafy::Decoder::decode_packet(AVPacket* packet, int* res) {
    std::unique_lock<std::mutex> lock1{lock};
    return decode_packet_internal(packet, res);
}

std::vector<std::shared_ptr<AVFrame>> libmediafy::Decoder::flush() {
    if (!initialized) return std::vector<std::shared_ptr<AVFrame>>();
    std::unique_lock<std::mutex> lock1{lock};

    std::vector<std::shared_ptr<AVFrame>> frames;

    int res;
    avcodec_flush_buffers(codec_ctx);
    frames = decode_packet_internal(nullptr, &res);
    return frames;
}

std::vector<std::shared_ptr<AVFrame>>
libmediafy::Decoder::decode_packet_internal(AVPacket *packet, int* result) {
    std::vector<std::shared_ptr<AVFrame>> frames{};
    if (!initialized) return frames;
    *result = avcodec_send_packet(codec_ctx, packet);
    if (*result < 0) {
//        printf("Got packet sending error: %s", av_err2str((*result)));
    }

    std::shared_ptr<AVFrame> frame{av_frame_alloc(), FrameDeleter()};
    while (avcodec_receive_frame(codec_ctx, frame.get()) >= 0) {
        frames.push_back(frame);
        frame = std::shared_ptr<AVFrame>(av_frame_alloc(), FrameDeleter());
    }

    return frames;
}

void libmediafy::Decoder::create() {
    printf("Decoder name: %s, ID: %d\n", avcodec_get_name(codec_id), codec_id);
    bool found_mediacodec = true;
    if (vout_surface && !ignore_hardware) {
        switch (codec_id) {
            case AV_CODEC_ID_H264:
                decoder = avcodec_find_decoder_by_name("h264_mediacodec");
                if (decoder) printf("H264 Decoder for mediacodec is used!");
                break;
            case AV_CODEC_ID_HEVC:
                printf("HEVC is used!");
                decoder = avcodec_find_decoder_by_name("hevc_mediacodec");
                break;
            case AV_CODEC_ID_MPEG2VIDEO:
                decoder = avcodec_find_decoder_by_name("mpeg2_mediacodec");
                break;
            case AV_CODEC_ID_MPEG4:
                decoder = avcodec_find_decoder_by_name("mpeg4_mediacodec");
            case AV_CODEC_ID_VP8:
                decoder = avcodec_find_decoder_by_name("vp8_mediacodec");
                break;
            case AV_CODEC_ID_VP9:
                decoder = avcodec_find_decoder_by_name("vp9_mediacodec");
                break;
        }
    }

    if (!decoder) {
        found_mediacodec = false;
        decoder = avcodec_find_decoder(codec_id);
        if (!decoder) {
            printf("Couldn't find decoder from here!\n");
        }
    }

    if (decoder && found_mediacodec) {
        output_format = AV_PIX_FMT_MEDIACODEC;
        codec_ctx = avcodec_alloc_context3(decoder);
        codec_ctx->thread_count = 2;
        AVBufferRef* device_ref = av_hwdevice_ctx_alloc(AV_HWDEVICE_TYPE_MEDIACODEC);
        AVHWDeviceContext* device_ctx = (AVHWDeviceContext*) device_ref->data;
        AVMediaCodecDeviceContext* hwctx = (AVMediaCodecDeviceContext*) device_ctx->hwctx;
        hwctx->surface = vout_surface;
        av_hwdevice_ctx_init(device_ref);
        codec_ctx->hw_device_ctx = device_ref;
    } else if (decoder) {
        printf("Found software decoder!\n Allocating context...\n");
        codec_ctx = avcodec_alloc_context3(decoder);
        if (!codec_ctx) printf("Could not allocate codec context!\n");
    } else {
        printf("No usable decoder was found!\n");
    }

        printf("Decoder finished create!\n");
}
#ifdef ANDROID
JNIEnv *libmediafy::Decoder::get_env() {
    JavaVM* vm = (JavaVM*)av_jni_get_java_vm(nullptr);
    JNIEnv* env{nullptr};
    if (vm->GetEnv((void**)&env, JNI_VERSION_1_6) == JNI_EDETACHED) {
        vm->AttachCurrentThread(&env, nullptr);
    }
    return env;
}
#endif

libmediafy::Decoder::~Decoder() {
    release();
#ifdef ANDROID
    get_env()->DeleteGlobalRef((jobject)vout_surface);
#endif
}
