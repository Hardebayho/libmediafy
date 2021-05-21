//
// Created by adebayo on 4/3/21.
//

#ifndef LIBMEDIAFYANDROID_MEDIAPLAYER_H
#define LIBMEDIAFYANDROID_MEDIAPLAYER_H
#include "Decoder.h"
#include "Media.h"

#include <functional>

extern "C" {
#include <libswresample/swresample.h>
#include <libavcodec/jni.h>
};

#include "VideoOutput.h"
#include "SubtitlesManager.h"
#include "concurrent_queue.h"

#include <deque>

#include <condition_variable>

#include "TSQueue.h"
#include "AudioOutput.h"
#include "AudioOutput.h"
#include "Deleters.h"
#include "Gain.h"
#include "Equalizer.h"
#include "ACompressor.h"

namespace libmediafy {
class MediaPlayer : public AudioCallbackHandler {
public:
    MediaPlayer();

    bool set_data_source(const char* url);

    MediaPlayer(const MediaPlayer&) = delete;

    bool get_buffer(int num_frames, unsigned int bytes_per_frame, void* buffer) override;

    ~MediaPlayer() {
        release();
    }
    bool has_media() const { return initialized; }
    void play();
    void pause();
    void stop();
    void seek(double position);

    std::shared_ptr<Media> get_media() {
        return current_media;
    }

    void release();

    void set_vout_surface(void* vout) {
        vout_renderer.set_output_surface(vout);
    }

    double get_current_time();

#ifdef ANDROID
    JNIEnv* get_env() {
        JNIEnv* env{nullptr};
        JavaVM* vm = static_cast<JavaVM*>(av_jni_get_java_vm(nullptr));
        if (vm->GetEnv((void**)&env, JNI_VERSION_1_6) == JNI_EDETACHED) {
            vm->AttachCurrentThread(&env, nullptr);
        }
        return env;
    }
#endif

    void set_subtitles_textview(void* sub_view) {
        vout_renderer.set_subtitles_view(sub_view);
    }

    /// Whether this player is currently playing
    bool is_playing() { return playing; }

    float get_volume() { return volume ? volume->get_multiplier() : 1.0; }
    void set_volume(float vol) {
        if (!volume) return;
        if (vol > 4.0) vol = 4.0f;
        if (vol < 0.0) vol = 0.0f;
        volume->set_multiplier(vol);
    }

    void activate_equalizer() {
        printf("Equalizer activated!\n");
        equalizer_active = true;
    }

    void deactivate_equalizer() {
        printf("Equalizer deactivated!\n");
        equalizer_active = false;
    }

    bool is_equalizer_active() {
        return equalizer_active;
    }

    std::shared_ptr<Equalizer> get_equalizer() const {
        return equalizer;
    }

private:
    bool initialized{false};
    const char* url{nullptr};
    std::atomic<double> current_time{0};
    std::shared_ptr<Media> current_media{nullptr};
    int audio_stream_index{-1};
    int video_stream_index{-1};
    Decoder* audio_decoder{nullptr};
    Decoder* video_decoder{nullptr};
    std::atomic_bool playing{false};
    std::atomic<double> last_audio_pts{0};

    void clear_buffers() {
        audio_packets.clear();
        video_packets.clear();
        audio_frames.clear();
        video_frames.clear();
    }

    const unsigned int MAX_PACKETS = 50;

    std::condition_variable audio_packets_cond{};
    TSQueue<std::shared_ptr<AVPacket>> audio_packets{MAX_PACKETS};

    std::condition_variable video_packets_cond{};
    TSQueue<std::shared_ptr<AVPacket>> video_packets{MAX_PACKETS};

    const unsigned int MAX_FRAMES = 20;
    std::condition_variable audio_frames_cond{};
    TSQueue<std::shared_ptr<AVFrame>> audio_frames{MAX_FRAMES};

    std::condition_variable video_frames_cond{};
    TSQueue<std::shared_ptr<AVFrame>> video_frames{MAX_FRAMES};

    std::thread demuxer_thread{};
    void demux_thread();
    std::mutex demux_lock{};
    std::condition_variable demux_cond{};
    std::atomic_bool demuxing{false};

    std::thread video_output_thread{};
    void vout_thread();
    // update to the latest frame cause we just seek
    std::atomic_bool seek_update_frame{false};

    std::atomic<double> last_video_pts{0};
    int64_t last_video_time{0};

    AudioOutputBuilder audio_output_builder{};
    std::unique_ptr<AudioOutput> audio_output{nullptr};
    SwrContext* resampler{nullptr};

    /// Called to set the current time
    void report_time_changed(double time);
    std::unique_ptr<AudioOutput> aout{nullptr};
    VideoOutput vout_renderer;

    int subtitles_stream_index{};
    SubtitlesManager subtitlesManager{};
    std::mutex vout_thread_lock{};

    std::condition_variable playing_cond{};

    std::atomic_bool seek_requested{false};
    std::atomic<double> seek_position{0.0};

    std::thread video_dec_th{};
    void video_decoder_thread();

    std::thread audio_dec_th{};
    void audio_decoder_thread();

    std::atomic_bool finished_pb{false};
    std::atomic_bool reported_pb_finished{false};

    std::mutex video_dec_lock{};
    std::mutex audio_dec_lock{};

    // Lock for the global player state
    std::mutex global_lock{};
    std::mutex audio_output_lock{};

    ACompressor* compressor;
    std::shared_ptr<Equalizer> equalizer{};
    std::shared_ptr<Gain> volume{};

    std::atomic_bool equalizer_active{false};

    void filter_frame(std::shared_ptr<AVFrame> frame);
};

}


#endif //LIBMEDIAFYANDROID_MEDIAPLAYER_H
