//
// Created by adebayo on 5/10/21.
//

#include "Resampler.h"

libmediafy::Resampler::Resampler(int in_channels, int in_sample_rate, AVSampleFormat in_sample_fmt, int out_channels, int out_sample_rate, AVSampleFormat out_sample_fmt) : out_fmt(out_sample_fmt), out_sample_rate(out_sample_rate) {
    initialized = false;
    int64_t in_ch_layout = av_get_default_channel_layout(in_channels);
    int64_t out_ch_layout = av_get_default_channel_layout(out_channels);
    ctx = swr_alloc_set_opts(nullptr, out_ch_layout, out_sample_fmt, out_sample_rate, in_ch_layout, in_sample_fmt, in_sample_rate, 0, nullptr);
    initialized = ctx;
}

libmediafy::Resampler::~Resampler() {
    swr_free(&ctx);
}

std::shared_ptr<AVFrame> libmediafy::Resampler::resample(std::shared_ptr<AVFrame> frame) {
    if (!frame || !initialized) return nullptr;
    AVFrame* resampled_frame = av_frame_alloc();
    if (!resampled_frame) return nullptr;

    resampled_frame->sample_rate = out_sample_rate;
    resampled_frame->format = out_fmt;
    resampled_frame->nb_samples = frame->nb_samples;
    resampled_frame->pts = frame->pts;
    resampled_frame->channel_layout = frame->channel_layout;

    int ret = swr_convert_frame(ctx, resampled_frame, frame.get());

    if (ret < 0) {
        av_frame_free(&resampled_frame);
        return nullptr;
    }

    return std::shared_ptr<AVFrame>(resampled_frame);
}
