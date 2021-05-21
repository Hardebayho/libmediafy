//
// Created by adebayo on 5/14/21.
//

#include "ACompressor.h"

extern "C" {
#include <libavfilter/avfilter.h>
#include <libavfilter/buffersink.h>
#include <libavfilter/buffersrc.h>
};

#include "Deleters.h"

namespace libmediafy {
    ACompressor::ACompressor(AVRational time_base, int sample_rate, int sample_format,
                             int64_t channel_layout) {
        graph = avfilter_graph_alloc();
        const AVFilter* src_filter = avfilter_get_by_name("abuffer");
        const AVFilter* sink_filter = avfilter_get_by_name("abuffersink");

        char args[512];

        snprintf(args, sizeof(args), "time_base=%d/%d:sample_rate=%d:sample_fmt=%s:channel_layout=0x%" PRIx64, time_base.num, time_base.den, sample_rate, av_get_sample_fmt_name((AVSampleFormat)sample_format), channel_layout);

        int ret = avfilter_graph_create_filter(&src, src_filter, "in", args, NULL, graph);
        ret = avfilter_graph_create_filter(&sink, sink_filter, "out", NULL, NULL, graph);

        const AVFilter* filter = avfilter_get_by_name("stereowiden");
        ret = avfilter_graph_create_filter(&acompressor, filter, "acompressor", nullptr, nullptr, graph);

        avfilter_init_str(acompressor, nullptr);

        ret = avfilter_link(src, 0, acompressor, 0);
        if (ret < 0) {
            printf("Couldn't link src to acompressor!");
        }
        avfilter_link(acompressor, 0, sink, 0);
        if (ret < 0) {
            printf("Couldn't link acompressor to sink!");
        }

        ret = avfilter_graph_config(graph, nullptr);
        if (ret < 0) {
            printf("Couldn't config acompressor graph!");
            abort();
        }
    }

    std::vector<std::shared_ptr<AVFrame>> ACompressor::filter(std::shared_ptr <AVFrame> frame) {
        std::vector<std::shared_ptr<AVFrame>> frames;
        if (av_buffersrc_add_frame(src, frame.get()) < 0) {
            printf("Couldn't add frame to buffersrc!");
        }

        AVFrame* fr = av_frame_alloc();
        while (av_buffersink_get_frame(sink, fr) >= 0) {
            std::shared_ptr<AVFrame> fr2{fr, FrameDeleter()};
            frames.push_back(fr2);
            fr = av_frame_alloc();
        }
        av_frame_free(&fr);
        return frames;
    }
}

