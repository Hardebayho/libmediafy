//
// Created by adebayo on 5/7/21.
//

#ifndef LIBMEDIAFYANDROID_AUDIOOUTPUT_H
#define LIBMEDIAFYANDROID_AUDIOOUTPUT_H

#include <cstdint>
#include <functional>
#include <memory>
#include <vector>

extern "C" {
#include <libavutil/samplefmt.h>
};

namespace libmediafy {

    class AudioCallbackHandler {
    public:
        virtual bool get_buffer(int num_frames, unsigned int bytes_per_frame, void* buffer) = 0;
    };

    class PacketDeleter;
    class FrameDeleter;

    class AudioOutput;

    /// Class used to create an audio output device
    class AudioOutputBuilder {
    public:
        AudioOutputBuilder() = default;
        AudioOutputBuilder& set_sample_format(AVSampleFormat format);
        AudioOutputBuilder& set_sample_rate(int rate);
        AudioOutputBuilder& set_channels(int count);
        /// Function that returns audio callback
        AudioOutputBuilder& set_audio_callback_handler(AudioCallbackHandler* handler);
        std::unique_ptr<AudioOutput> create();
    private:
        int channels{-1};
        int sample_rate{-1};
        AVSampleFormat sample_format{AV_SAMPLE_FMT_NONE};
        AudioCallbackHandler* callback_handler{};
    };

    class AudioOutput {
    public:
        /// Start running
        virtual void start() = 0;

        /// Stop running
        virtual void stop() = 0;

        /// Flush the stream
        virtual void flush() = 0;

        /// Close the stream
        virtual void close() = 0;

        virtual bool is_started() = 0;

        int get_sample_rate() const {
            return sample_rate;
        }

        int get_bytes_per_frame() const {
            int size = sample_format == AV_SAMPLE_FMT_FLT ? sizeof(float) : sizeof(int16_t);
            return size * channels;
        }

        AVSampleFormat get_sample_format() const {
            return sample_format;
        }

        int get_channels() const {
            return channels;
        }

        bool is_initialized() const {
            return initialized;
        }

        const AudioCallbackHandler* get_callback_func() const {
            return callback_handler;
        }

        /// A simple virtual destructor, for de-initialization
        virtual ~AudioOutput() {}

        AudioCallbackHandler* callback_handler{nullptr};

    protected:
        AudioOutput(int sampleRate, AVSampleFormat sampleFormat, int channels, AudioCallbackHandler* handler);
        int sample_rate;
        AVSampleFormat sample_format;
        int channels;
        bool initialized{false};
        friend class AudioOutputBuilder;
    };
}

#endif //LIBMEDIAFYANDROID_AUDIOOUTPUT_H
