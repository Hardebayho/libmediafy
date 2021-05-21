//
// Created by adebayo on 5/10/21.
//

#ifndef LIBMEDIAFYANDROID_ACOMPRESSOR_H
#define LIBMEDIAFYANDROID_ACOMPRESSOR_H

#include <memory>

extern "C" {
#include <libavfilter/avfilter.h>
#include <libavutil/frame.h>
};
#include <vector>

namespace libmediafy {
    class ACompressor {
    public:
        ACompressor(AVRational time_base, int sample_rate, int sample_format, int64_t channel_layout);
        std::vector<std::shared_ptr<AVFrame>> filter(std::shared_ptr<AVFrame> frame);

    private:
        AVFilterContext* acompressor{};
        AVFilterContext* src{};
        AVFilterContext* sink{};
        AVFilterGraph* graph{};
    };
}

#endif //LIBMEDIAFYANDROID_ACOMPRESSOR_H
