//
// Created by adebayo on 5/10/21.
//

#ifndef LIBMEDIAFYANDROID_RESAMPLER_H
#define LIBMEDIAFYANDROID_RESAMPLER_H

extern "C" {
#include <libswresample/swresample.h>
};
#include <memory>
#include "Deleters.h"

namespace libmediafy {
    class Resampler {
    public:
        explicit Resampler(int in_channels, int in_sample_rate, AVSampleFormat in_sample_fmt,
                int out_channels, int out_sample_rate, AVSampleFormat out_sample_fmt);

        std::shared_ptr<AVFrame> resample(std::shared_ptr<AVFrame> frame);

        bool is_initialized() { return initialized; }

        ~Resampler();
    private:
        SwrContext* ctx{nullptr};
        bool initialized{false};
        AVSampleFormat out_fmt;
        int out_sample_rate;
    };
}


#endif //LIBMEDIAFYANDROID_RESAMPLER_H
