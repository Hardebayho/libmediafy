//
// Created by adebayo on 4/12/21.
//

#ifndef LIBMEDIAFYANDROID_SUBTITLESMANAGER_H
#define LIBMEDIAFYANDROID_SUBTITLESMANAGER_H
#define __STDC_CONSTANT_MACROS
extern "C" {
#include <libavutil/avutil.h>
#include <libavformat/avformat.h>
};
#include <set>
#include <string>
#include <memory>

namespace libmediafy {
    struct Cue {
        double from;
        double to;
        std::string data;
        bool operator<(const Cue& rhs) const {
            return from < rhs.from && to < rhs.to;
        }
    };

    class SubtitlesManager {
    public:
        SubtitlesManager() = default;
        ~SubtitlesManager() {
            flush();
        }
        void set_time_base(double tb) {
            time_base = tb;
        }
        void queue_packet(std::shared_ptr<AVPacket>& packet);
        bool get_cue(double time, Cue* ret_queue);
        void flush();

        bool parse_srt(const char* subtitles);
    private:
        double time_base{};
        std::set<Cue> cues;
    };
}

#endif //LIBMEDIAFYANDROID_SUBTITLESMANAGER_H
