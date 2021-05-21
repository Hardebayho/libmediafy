//
// Created by adebayo on 4/3/21.
//

#ifndef LIBMEDIAFYANDROID_MEDIA_H
#define LIBMEDIAFYANDROID_MEDIA_H

#include <string>

extern "C" {
#include <libavformat/avformat.h>
}

#include <mutex>
#include <memory>
#include <vector>
#include "Deleters.h"

namespace libmediafy {

    struct StreamDescription {
        std::shared_ptr<AVCodecParameters> description;
        std::shared_ptr<AVDictionary> metadata;
    };

    class Media {
    public:
        std::shared_ptr<AVPacket> read();
        bool seek(int64_t position);
        int64_t duration();
        bool write(AVPacket* packet);
        bool requires_global_header();
        bool flush();
        const AVInputFormat* iformat();
        const AVOutputFormat* oformat();
        int nb_streams();
        const AVStream* const stream(int index);
        AVPacket* get_album_art();
        ~Media();
    private:
        friend class MediaBuilder;
        Media() = default;
        std::mutex media_lock{};
        std::shared_ptr<AVFormatContext> format_ctx{};
        bool muxer;
    };

    class MediaBuilder {
    public:
        MediaBuilder() = default;
        std::shared_ptr<Media> create();
        MediaBuilder* set_url(std::string url) {
            this->url = url;
            return this;
        }
        MediaBuilder* set_muxer(bool muxer) {
            this->muxer = muxer;
            return this;
        }
        MediaBuilder* set_flags(int flags) {
            this->flags = flags;
            return this;
        }
        MediaBuilder* set_stream_descriptions(std::vector<StreamDescription> descriptions) {
            this->descriptions = descriptions;
            return this;
        }

        MediaBuilder* set_output_format(AVOutputFormat* format) {
            fmt = format;
            return this;
        }
    private:
        std::string url{};
        bool muxer{false};
        int flags{0};
        AVOutputFormat* fmt;
        std::vector<StreamDescription> descriptions;
    };
}

#endif //LIBMEDIAFYANDROID_MEDIA_H
