//
// Created by adebayo on 4/12/21.
//

#include "SubtitlesManager.h"

extern "C" {
#include <libavutil/mem.h>
}

#include <algorithm>

void libmediafy::SubtitlesManager::queue_packet(std::shared_ptr<AVPacket>& packet) {
    if (!packet->data || packet->size <= 0) {
        return;
    }
    double start = packet->pts * time_base;
    double end = start + packet->duration * time_base;
    Cue cue {
            .from = start,
            .to = end,
    };
    cue.data = (const char*)packet->data;

    cues.insert(cue);
}

bool libmediafy::SubtitlesManager::get_cue(double time, libmediafy::Cue *ret_queue) {
    auto is_available = [&](Cue data) {
        return time >= data.from && time < data.to;
    };

    auto iter = std::find_if(cues.begin(), cues.end(), is_available);
    if (iter != cues.end()) *ret_queue = *iter;

    return iter != cues.end();
}

void libmediafy::SubtitlesManager::flush() {
    cues.clear();
}

bool libmediafy::SubtitlesManager::parse_srt(const char *subtitles) {
    return false;
}
