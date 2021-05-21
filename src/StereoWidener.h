//
// Created by adebayo on 5/10/21.
//

#ifndef LIBMEDIAFYANDROID_STEREOWIDENER_H
#define LIBMEDIAFYANDROID_STEREOWIDENER_H

extern "C" {
#include <libavfilter/avfilter.h>
#include <libavfilter/buffersink.h>
#include <libavfilter/buffersrc.h>
#include <libavutil/opt.h>
};

namespace libmediafy {
    class StereoWidener {
    public:
        StereoWidener(AVRational time_base, int sample_rate, int sample_format, int64_t channel_layout) {
            filter_graph = avfilter_graph_alloc();
            const AVFilter *abuffersrc  = avfilter_get_by_name("abuffer");
            const AVFilter *abuffersink = avfilter_get_by_name("abuffersink");

            char args[512];

            snprintf(args, sizeof(args),
                     "time_base=%d/%d:sample_rate=%d:sample_fmt=%s:channel_layout=0x%" PRIx64,
                     time_base.num, time_base.den, sample_rate,
                     av_get_sample_fmt_name((AVSampleFormat)sample_format), channel_layout);

            int ret = avfilter_graph_create_filter(&buffersrc_ctx, abuffersrc, "in", args, NULL, filter_graph);
            ret = avfilter_graph_create_filter(&buffersink_ctx, abuffersink, "out", NULL, NULL, filter_graph);
        }

    private:
        AVFilterGraph* filter_graph{};
        AVFilterContext* buffersrc_ctx{nullptr};
        AVFilterContext* buffersink_ctx{nullptr};
        AVFilterContext* ctx{nullptr};
    };
}

#endif //LIBMEDIAFYANDROID_STEREOWIDENER_H
