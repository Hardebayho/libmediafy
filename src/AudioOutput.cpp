//
// Created by adebayo on 5/7/21.
//

#include "AudioOutput.h"

#include <atomic>

extern "C" {
#include <libavutil/time.h>
};
#define MINIAUDIO_IMPLEMENTATION
#include "miniaudio.h"

#ifndef MA_SUCCESS
#define MA_SUCCESS 0
#endif

void data_callback(ma_device* device, void* output, const void* input, ma_uint32 frames);

class LibmediafyAudioOutput : public libmediafy::AudioOutput {
public:
    LibmediafyAudioOutput(int sampleRate, AVSampleFormat sampleFormat, int channels, libmediafy::AudioCallbackHandler* handler) : AudioOutput(sampleRate, sampleFormat, channels, handler) {
        ma_format fmt = ma_format_f32;
        if (sampleFormat == AV_SAMPLE_FMT_S16) fmt = ma_format_s16;
        ma_device_config config = ma_device_config_init(ma_device_type_playback);
        config.playback.format = fmt;
        config.playback.channels = channels;
        config.sampleRate = sampleRate;
        config.dataCallback = data_callback;
        config.pUserData = this;
        initialized = ma_device_init(nullptr, &config, &device) == MA_SUCCESS;
    }

    void start() override {
        if (!initialized) return;
        started = true;
        ma_device_start(&device);
    }

    void stop() override {
        if (!initialized) return;
        started = false;
        ma_device_stop(&device);
    }

    void close() override {
        if (!initialized) return;
        started = false;
        ma_device_stop(&device);
    }

    void flush() override {
        if (!initialized) return;
        // Nothing TODO
    }

    bool is_started() override {
        return started;
    }

    virtual ~LibmediafyAudioOutput() {
        ma_device_uninit(&device);
    }

private:
    ma_device device;
    std::atomic_bool started{false};
};

libmediafy::AudioOutputBuilder&
libmediafy::AudioOutputBuilder::set_sample_format(AVSampleFormat format) {
    sample_format = format;
    return *this;
}

libmediafy::AudioOutputBuilder &libmediafy::AudioOutputBuilder::set_sample_rate(int rate) {
    sample_rate = rate;
    return *this;
}

libmediafy::AudioOutputBuilder &libmediafy::AudioOutputBuilder::set_channels(int count) {
    channels = count;
    return *this;
}

libmediafy::AudioOutputBuilder&
libmediafy::AudioOutputBuilder::set_audio_callback_handler(libmediafy::AudioCallbackHandler* handler) {
    callback_handler = handler;
    return *this;
}

std::unique_ptr<libmediafy::AudioOutput> libmediafy::AudioOutputBuilder::create() {
    if (channels < 0 || sample_rate < 0 || sample_format == AV_SAMPLE_FMT_NONE)
        return nullptr;
    if (!callback_handler) return nullptr;
    AudioOutput* aout{nullptr};

    aout = new LibmediafyAudioOutput(sample_rate, sample_format, channels, callback_handler);

    if (!aout) return nullptr;

    if (!aout->initialized) {
        delete aout;
        aout = nullptr;
    }
    return std::unique_ptr<libmediafy::AudioOutput>(aout);
}

libmediafy::AudioOutput::AudioOutput(int sampleRate, AVSampleFormat sampleFormat,
                                     int channels, libmediafy::AudioCallbackHandler* handler) : sample_rate(sampleRate), sample_format(sampleFormat), channels(channels), callback_handler(handler) {}

void data_callback(ma_device* device, void* output, const void* input, ma_uint32 frames) {
    libmediafy::AudioOutput* aout = reinterpret_cast<libmediafy::AudioOutput*>(device->pUserData);
    unsigned int bytes_per_frame = device->playback.format == ma_format_f32 ? sizeof(float) * device->playback.channels : sizeof(int16_t) * device->playback.channels;
    aout->callback_handler->get_buffer(frames, bytes_per_frame, output);
}
