#include <memory>


#include "api.h"
#include "MediaPlayer.h"
#include "Encoder.h"
#include "Deleters.h"

extern "C" {
#include <libswscale/swscale.h>
#include <libavutil/audio_fifo.h>
}

#include <map>

extern "C" {

///////// IMPLEMENTATIONS

libmediafy_player libmediafy_player_new() {
    auto player = new(std::nothrow) libmediafy::MediaPlayer();
    if (!player) return nullptr;
    return player;
}


int libmediafy_player_set_data_source(libmediafy_player player_t, const char* url) {
    auto* player = (libmediafy::MediaPlayer*)player_t;
    return player->set_data_source(url) ? 0 : -1;
}


int libmediafy_player_has_media(libmediafy_player player_t) {
    auto* player = (libmediafy::MediaPlayer*)player_t;
    return player->has_media() ? 1 : 0;
}


int libmediafy_player_delete(libmediafy_player* player) {
    if (player) delete (libmediafy::MediaPlayer*)(*player);
    *player = nullptr;
    return 0;
}


int libmediafy_player_play(libmediafy_player player_t) {
    auto* player = (libmediafy::MediaPlayer*)player_t;
    player->play();
    return 0;
}


int libmediafy_player_pause(libmediafy_player player_t) {
    auto* player = (libmediafy::MediaPlayer*)player_t;
    player->pause();
    return 0;
}


int libmediafy_player_stop(libmediafy_player player_t) {
    auto* player = (libmediafy::MediaPlayer*)player_t;
    player->stop();
    return 0;
}


int libmediafy_player_is_playing(libmediafy_player player_t) {
    auto* player = (libmediafy::MediaPlayer*)player_t;
    return player->is_playing() ? 1 : 0;
}


int libmediafy_player_seek(libmediafy_player player_t, double pos_secs) {
    auto* player = (libmediafy::MediaPlayer*)player_t;
    player->seek(pos_secs);
    return 0;
}


float libmediafy_player_get_volume(libmediafy_player player_t) {
    auto* player = (libmediafy::MediaPlayer*)player_t;
    return player->get_volume();
}

int libmediafy_player_set_volume(libmediafy_player player_t, float value) {
    auto* player = (libmediafy::MediaPlayer*)player_t;
    player->set_volume(value);
    return 0;
}


double libmediafy_player_get_duration(libmediafy_player player_t) {
    auto* player = (libmediafy::MediaPlayer*)player_t;
    return player->get_media()->duration() / 1000.0;
}


double libmediafy_player_get_time(libmediafy_player player_t) {
    auto* player = (libmediafy::MediaPlayer*)player_t;
    return player->get_current_time();
}

LMDFY_EXPORT int libmediafy_player_get_equalizer_bands(libmediafy_player player_t) {
    auto* player = (libmediafy::MediaPlayer*)player_t;
    return player->get_equalizer()->bands_count();
}

LMDFY_EXPORT int libmediafy_player_enable_equalizer(libmediafy_player player_t) {
    auto* player = (libmediafy::MediaPlayer*)player_t;
    player->activate_equalizer();
    return 0;
}

LMDFY_EXPORT int libmediafy_player_disable_equalizer(libmediafy_player player_t) {
    auto* player = (libmediafy::MediaPlayer*)player_t;
    player->deactivate_equalizer();
    return 0;
}

LMDFY_EXPORT int libmediafy_player_is_equalizer_enabled(libmediafy_player player_t) {
    auto* player = (libmediafy::MediaPlayer*)player_t;
    return player->is_equalizer_active() ? 1 : 0;
}

LMDFY_EXPORT float libmediafy_player_get_equalizer_frequency(libmediafy_player player_t, int index) {
    auto* player = (libmediafy::MediaPlayer*)player_t;
    return player->get_equalizer()->get_frequency(index);
}

LMDFY_EXPORT int libmediafy_player_set_equalizer_frequency(libmediafy_player player_t, int index, float value) {
    auto* player = (libmediafy::MediaPlayer*)player_t;
    player->get_equalizer()->set_frequency(index, value);
    return 0;
}

LMDFY_EXPORT int libmediafy_player_get_num_equalizer_presets(libmediafy_player player_t) {
    auto* player = (libmediafy::MediaPlayer*)player_t;
    return player->get_equalizer()->preset_count();
}

LMDFY_EXPORT const char* libmediafy_player_get_equalizer_preset_name(libmediafy_player player_t, int preset_index) {
    auto* player = (libmediafy::MediaPlayer*)player_t;
    return player->get_equalizer()->preset_name(preset_index).c_str();
}

LMDFY_EXPORT float libmediafy_player_get_equalizer_preset_frequency(libmediafy_player player_t, int preset_index, int freq_index) {
    auto player = (libmediafy::MediaPlayer*)player_t;
    return player->get_equalizer()->preset_frequency(preset_index, freq_index);
}

LMDFY_EXPORT float libmediafy_player_get_equalizer_preamp(libmediafy_player player_t) {
    auto* player = (libmediafy::MediaPlayer*)player_t;
    return player->get_equalizer()->get_preamp();
}

LMDFY_EXPORT int libmediafy_player_set_equalizer_preamp(libmediafy_player player_t, float preamp) {
    auto* player = (libmediafy::MediaPlayer*)player_t;
    player->get_equalizer()->set_preamp(preamp);
    return 0;
}

LMDFY_EXPORT int libmediafy_player_set_equalizer_preset(libmediafy_player player_t, int preset_index) {
    auto* player = (libmediafy::MediaPlayer*)player_t;
    player->get_equalizer()->set_params(player->get_equalizer()->preset(preset_index));
    return 0;
}

LMDFY_EXPORT int libmediafy_player_get_album_art(libmediafy_player player_t, libmediafy_image **out_frame) {
    auto* player = (libmediafy::MediaPlayer*)player_t;
    if (!player->has_media()) return -1;
    auto album_art = player->get_media()->get_album_art();
    if (album_art) {
        auto *frame = new(std::nothrow) libmediafy_image();
        memset(frame, 0, sizeof(*frame));
        frame->_internal_obj = album_art;
        frame->size = album_art->size;
        frame->buffer = album_art->data;
        *out_frame = frame;
        return 0;
    }
    return -1;
}

LMDFY_EXPORT int libmediafy_load_thumbnail(const char* path, int width, int height, libmediafy_image **out_frame) {
    // 1. Load the media
    auto media = libmediafy::MediaBuilder().set_muxer(false)->set_url(std::string(path))->create();
    // 2. Perform sanity checks
    if (!media || width < 0 || height < 0 || !out_frame) return -1;

    // 3. Get a video stream
    for (int i = 0; i < media->nb_streams(); i++) {
        auto stream = media->stream(i);
        if (stream->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {

            // 4. Find a decoder for the stream
            auto decoder = std::make_shared<libmediafy::Decoder>(stream->codecpar->codec_id, nullptr, true);
            if (!decoder->initialize(stream->codecpar)) {
                return -1;
            }

            // 5. Seek to some random location
            uint64_t time; // Use whatever random value it contains
            auto duration = media->duration();
            if (time >= duration) time %= duration;
            media->seek(time);

            // 6. Decode from there until a frame is found
            std::shared_ptr<AVPacket> packet;
            while ((packet = media->read())) {
                if (packet->stream_index == stream->index) {
                    int res;
                    auto frames = decoder->decode_packet(packet.get(), &res);
                    std::shared_ptr<AVFrame> frame;
                    if (!frames.empty()) {
                        frame = frames[0];
                    }

                    // 7. Rescale the frame to the specified thumbnail size
                    if (frame) {
                        SwsContext *rescaler = sws_getContext(frame->width, frame->height, (AVPixelFormat)frame->format, width, height, AV_PIX_FMT_YUVJ420P, SWS_BICUBIC, nullptr, nullptr, nullptr);

                        if (!rescaler) {
                            fprintf(stderr, "The rescaler cannot be initialized! Will be returning the frame as-is\n");
                        } else {
                            AVFrame* frame2 = av_frame_alloc();
                            if (!frame2) {
                                sws_freeContext(rescaler);
                                return -1;
                            }
                            frame2->width = width;
                            frame2->height = height;
                            frame2->format = AV_PIX_FMT_YUVJ420P;
                            av_frame_get_buffer(frame2, 0);
                            int ret = sws_scale(rescaler, frame->data, frame->linesize, 0, frame->height, frame2->data, frame2->linesize);
                            if (ret < 0) {
                                fprintf(stderr, "Couldn't scale video frame!\n");
                                sws_freeContext(rescaler);
                                av_frame_free(&frame2);
                            } else {
                                // Move the frame2 inside frame and free frame
                                // Do it properly. This looks clearer
                                frame.reset();
                                frame.reset(frame2, libmediafy::FrameDeleter());
                            }
                        }

                        // 8. Encode the received frame (mjpeg)
                        libmediafy::Encoder encoder{AV_CODEC_ID_MJPEG};
                        AVCodecParameters* params = avcodec_parameters_alloc();
                        params->width = frame->width;
                        params->height = frame->height;
                        params->format = frame->format;
                        params->codec_type = AVMEDIA_TYPE_VIDEO;
                        encoder.set_time_base({1, 1});
                        if (!encoder.initialize(params)) {
                            avcodec_parameters_free(&params);
                            fprintf(stderr, "Couldn't initialize the encoder!");
                            return -1;
                        }
                        avcodec_parameters_free(&params);
                        auto packets = encoder.encode_frame(frame.get(), &res);
                        if (!packets.empty()) {
                            // 9. Create an image from the encoded frame and return it
                            auto *image = new(std::nothrow) libmediafy_image();
                            if (!image) {
                                return -1;
                            }
                            image->_internal_obj = packets[0].get();
                            image->buffer = packets[0]->data;
                            image->size = packets[0]->size;
                            *out_frame = image;
                            return 0;
                        } else {
                            fprintf(stderr, "Something's wrong! The mjpeg encoder didn't give us back an mjpeg packet! Res is %d Bailing...\n", res);
                            return -1;
                        }
                    }
                }
            }
        }
    }

    return -1;
}

LMDFY_EXPORT int libmediafy_load_album_art(const char *path, libmediafy_image **out_frame) {
    auto media = libmediafy::MediaBuilder().set_muxer(false)->set_url(std::string(path))->create();
    if (!media) return -1;

    auto packet = media->get_album_art();
    if (packet) {
        auto *image = new(std::nothrow) libmediafy_image();
        if (!image) {
            av_packet_free(&packet);
            return -1;
        }
        image->_internal_obj = packet;
        image->buffer = packet->data;
        image->size = packet->size;
        *out_frame = image;
    }
    return 0;
}

LMDFY_EXPORT int libmediafy_load_video_frame(const char *path, uint64_t pos_millis, libmediafy_image **out_frame) {
    // 1. Load the media
    auto media = libmediafy::MediaBuilder().set_muxer(false)->set_url(std::string(path))->create();
    // 2. Perform sanity checks
    if (!media || !out_frame) return -1;
    if (media->duration() < (pos_millis)) return -1;

    double pos_secs = (double)pos_millis / 1000.0;

    // 3. Get a video stream
    for (int i = 0; i < media->nb_streams(); i++) {
        auto stream = media->stream(i);
        if (stream->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {

            // 4. Find a decoder for the stream
            auto decoder = std::make_shared<libmediafy::Decoder>(stream->codecpar->codec_id, nullptr, true);
            if (!decoder->initialize(stream->codecpar)) {
                return -1;
            }

            // 5. Seek to user-specified position
            uint64_t time = pos_secs * AV_TIME_BASE;
            auto duration = media->duration();
            if (time >= duration) time %= duration;
            media->seek(time);

            // 6. Decode from there until a frame is found
            std::shared_ptr<AVPacket> packet;
            while ((packet = media->read())) {
                if (packet->stream_index == stream->index) {
                    int res;
                    auto frames = decoder->decode_packet(packet.get(), &res);
                    std::shared_ptr<AVFrame> frame;
                    if (!frames.empty()) {
                        frame = frames[0];
                    }

                    if (frame) {
                        // Optional: Rescale if the frame's format is not yuvj420p
                        if (frame->format  != AV_PIX_FMT_YUVJ420P) {
                            SwsContext *rescaler = sws_getContext(frame->width, frame->height, (AVPixelFormat)frame->format, frame->width, frame->height, AV_PIX_FMT_YUVJ420P, SWS_BICUBIC, nullptr, nullptr, nullptr);
                            if (!rescaler) {
                                return -1;
                            }

                            AVFrame* frame2 = av_frame_alloc();
                            if (!frame) {
                                sws_freeContext(rescaler);
                                return -1;
                            }
                            frame2->width = frame->width;
                            frame2->height = frame->height;
                            frame2->format = AV_PIX_FMT_YUVJ420P;
                            if (av_frame_get_buffer(frame2, 0) < 0) {
                                sws_freeContext(rescaler);
                                av_frame_free(&frame2);
                                return -1;
                            }

                            int ret;
                            if ((ret = sws_scale(rescaler, frame->data, frame->linesize, 0, frame->height, frame2->data, frame2->linesize)) < 0) {
                                char errorLog[512];
                                fprintf(stderr, "Couldn't scale frame! Error: %s\n", av_make_error_string(errorLog, 512, ret));
                                av_frame_free(&frame2);
                                sws_freeContext(rescaler);
                                return -1;
                            }

                            frame.reset();
                            frame.reset(frame2, libmediafy::FrameDeleter());
                        }

                        // 7. Encode the received frame (mjpeg)
                        libmediafy::Encoder encoder{AV_CODEC_ID_MJPEG};
                        encoder.set_time_base({1, 1});
                        AVCodecParameters* params = avcodec_parameters_alloc();
                        params->width = frame->width;
                        params->height = frame->height;
                        params->format = frame->format;
                        params->codec_type = AVMEDIA_TYPE_VIDEO;
                        params->codec_id = AV_CODEC_ID_MJPEG;
                        if (!encoder.initialize(params)) {
                            fprintf(stderr, "Couldn't initialize the encoder!");
                            return -1;
                        }
                        auto packets = encoder.encode_frame(frame.get(), &res);
                        if (!packets.empty()) {
                            // 8. Create an image from the encoded frame and return it
                            auto *image = new(std::nothrow) libmediafy_image();
                            if (!image) {
                                return -1;
                            }
                            image->_internal_obj = packets[0].get();
                            image->buffer = packets[0]->data;
                            image->size = packets[0]->size;
                            *out_frame = image;
                            return 0;
                        } else {
                            fprintf(stderr, "Something's wrong! The mjpeg encoder didn't give us back an mjpeg packet! Bailing...\n");
                            return -1;
                        }
                    }
                }
            }
        }
    }

    return -1;
}

LMDFY_EXPORT int libmediafy_free_image(libmediafy_image **frame) {
    auto *packet = (AVPacket*)(*frame)->_internal_obj;
    av_packet_free(&packet);
    delete (*frame);
    *frame = nullptr;
    return 0;
}

LMDFY_EXPORT int libmediafy_free_aframe(libmediafy_aframe **frame) {
    return 0;
}

/// An operation performed by the task
struct operation_t {
    virtual int progress() = 0;
    virtual void start() = 0;
};

struct libmediafy_task_t {
    virtual int progress() = 0;
    virtual int resume() = 0;
    virtual bool ready() = 0;
    virtual int pause() = 0;
    virtual int abort() = 0;
    virtual bool completed() = 0;
    virtual bool paused() = 0;
    virtual ~libmediafy_task_t() = default;
};

LMDFY_EXPORT int libmediafy_task_pause(libmediafy_task task) {
    auto task_t = reinterpret_cast<libmediafy_task_t*>(task);
    return task_t->pause();
}

LMDFY_EXPORT int libmediafy_task_is_paused(libmediafy_task task) {
    auto task_t = reinterpret_cast<libmediafy_task_t*>(task);
    return task_t->paused() ? 1 : 0;
}

LMDFY_EXPORT int libmediafy_task_resume(libmediafy_task task) {
    auto task_t = reinterpret_cast<libmediafy_task_t*>(task);
    return task_t->resume();
}

LMDFY_EXPORT int libmediafy_task_abort(libmediafy_task task) {
    auto task_t = reinterpret_cast<libmediafy_task_t*>(task);
    return task_t->abort();
}

LMDFY_EXPORT int libmediafy_task_progress(libmediafy_task task) {
    auto task_t = reinterpret_cast<libmediafy_task_t*>(task);
    return task_t->progress();
}

LMDFY_EXPORT int libmediafy_task_is_complete(libmediafy_task task) {
    auto task_t = reinterpret_cast<libmediafy_task_t*>(task);
    return task_t->completed();
}

LMDFY_EXPORT int libmediafy_task_free(libmediafy_task *task) {
    auto task_t = reinterpret_cast<libmediafy_task_t*>(*task);
    task_t->abort();
    delete task_t;
    return 0;
}

struct split_media_task : public libmediafy_task_t {
    
    split_media_task(const std::string& url, const std::string& output_dir) {
        AVIODirContext* ctx{nullptr};
        int ret = avio_open_dir(&ctx, output_dir.c_str(), nullptr);
        avio_close_dir(&ctx);
        if (ret < 0) {
            return;
        }

        // 1. Open the input file
        input_media = builder.set_url(url)
                            ->set_muxer(false)
                            ->create();

        if (!input_media) return;

        for (int i = 0; i < input_media->nb_streams(); i++) {
            auto stream = input_media->stream(i);

            std::vector<libmediafy::StreamDescription> descs;
            libmediafy::StreamDescription desc;
            desc.description.reset(avcodec_parameters_alloc(), libmediafy::AVCodecParamsDeleter());
            avcodec_parameters_copy(desc.description.get(), stream->codecpar);
            descs.push_back(desc);

            std::string ext = output_dir + "/";
            ext += std::to_string(i);

            switch (stream->codecpar->codec_id) {
                case AV_CODEC_ID_AAC:
                    ext += ".aac";
                break;
                case AV_CODEC_ID_HEVC:
                case AV_CODEC_ID_H264:
                case AV_CODEC_ID_MPEG4:
                case AV_CODEC_ID_MPEG2VIDEO:
                case AV_CODEC_ID_MPEG1VIDEO:
                    ext += ".mp4";
                break;
                case AV_CODEC_ID_FLAC:
                    ext += ".flac";
                break;
                case AV_CODEC_ID_MP3:
                    ext += ".mp3";
                break;
                case AV_CODEC_ID_OPUS:
                    ext += ".opus";
                break;
                case AV_CODEC_ID_MJPEG:
                    ext += ".jpg";
                break;
                case AV_CODEC_ID_VORBIS:
                    ext += ".ogg";
                break;
                default:
                    continue;
            }

            auto media = builder.set_muxer(true)
                                ->set_url(ext)
                                ->set_stream_descriptions(descs)
                                ->set_output_format(nullptr)
                                ->create();

            if (!media) {
                fprintf(stderr, "Couldn't create output media for ext: %s\n", ext.c_str());
                continue;
            }

            outputs.push_back(media);
            streams.push_back(stream);
        }
        // 4. Enumerate all the found streams. If no stream can be worked with, return
        if (outputs.empty()) return;
        // 6. Set appropriate initialization flags and return
        current_index = 0;
        cur_progress = 0;
        abort_req = false;

        th = std::thread(&split_media_task::processing_func, this);
        th.detach();
    }

    int num_streams() {
        std::unique_lock<std::mutex> lock2{lock};
        return outputs.size();
    }
    bool ready() override {
        std::unique_lock<std::mutex> lock2{lock};
        return current_index >= 0;
    }
    int progress() override {
        std::unique_lock<std::mutex> lock2{lock};
        return cur_progress;
    }
    int resume() override {
        std::unique_lock<std::mutex> lock2{lock};
        if (abort_req) return -1;
        is_paused = false;
        state_cond.notify_one();
        return 0;
    }
    int pause() override {
        std::unique_lock<std::mutex> lock2{lock};
        if (abort_req) return -1;
        is_paused = true;
        return 0;
    }
    int abort() override {
        std::unique_lock<std::mutex> lock2{lock};
        if (abort_req) return -1;
        abort_req = true;
        return 0;
    }
    bool completed() override {
        std::unique_lock<std::mutex> lock2{lock};
        return is_completed;
    }
    bool paused() override {
        std::unique_lock<std::mutex> lock2{lock};
        return is_paused;
    }
    ~split_media_task() override {
        abort();
    }

private:
    std::mutex lock{};
    libmediafy::MediaBuilder builder{};
    std::shared_ptr<libmediafy::Media> input_media;
    std::vector<std::shared_ptr<libmediafy::Media>> outputs{};
    std::vector<const AVStream*> streams{};
    int current_index{-1};
    int cur_progress{-1};
    bool is_completed{false};

    std::mutex state_lock{};
    std::atomic_bool is_paused{true};
    std::atomic_bool abort_req{true};
    std::condition_variable state_cond;
    std::thread th;

    void processing_func() {
        // This is where the logic happens
        using namespace std::chrono_literals;
        while (!abort_req) {
            std::unique_lock<std::mutex> lock2{state_lock};
            while (is_paused) {
                state_cond.wait_for(lock2, 100us);
                if (abort_req) return;
            }

            // The process is,
            // Read and write a frame, write the progress and then circle back on the loop
            // This is so that we can reliably support the pause-abort mechanism

            auto packet = input_media->read();
            if (!packet) {
                // This means that we've finished with the input media, we now need to flush the output media and we're done!
                for (auto& output : outputs) {
                    output->flush();
                }
                cur_progress = 100;
                abort_req = true;
                is_completed = true;
                break;
            }

            int i = -1;

            for (i = 0; i < streams.size(); i++) {
                auto stream = streams[i];
                if (stream->index == packet->stream_index) {
                    if (packet->pts != AV_NOPTS_VALUE) {
                        cur_progress = ((double)packet->pts / 1000.0) * 100.0;
                    }
                    packet->stream_index = 0;
                    if (!outputs[i]->write(packet.get())) {
                        fprintf(stderr, "Could not write output packet! Stream index: %d\n", stream->index);
                    }
                    break;
                }
            }
        }
end:
        is_completed = true;
    }
};

LMDFY_EXPORT int libmediafy_split_media(const char *url, const char *output_dir, int *num_streams, libmediafy_task *task) {
    libmediafy_task_t* task_t = new split_media_task(url, output_dir);
    if (!task_t->ready()) {
        delete task_t;
        return -1;
    }
    *task = (void*)task_t;
    *num_streams = ((split_media_task*)task_t)->num_streams();
    return task_t->resume();
}

/// A fixed-size audio fifo buffer that's also thread-safe
class TSFifo {
public:
    TSFifo(AVSampleFormat format, int channels, int max_samples) : max_samples(max_samples) {
        fifo = av_audio_fifo_alloc(format, channels, max_samples);
    }

    bool write(void** buffer, int nb_samples) {
        std::unique_lock<std::mutex> lock1{lock};
        if (av_audio_fifo_size(fifo) >= max_samples || av_audio_fifo_size(fifo) + nb_samples >= max_samples) {
            return false;
        }
        return av_audio_fifo_write(fifo, buffer, nb_samples);
    }

    int read(void** buffer, int nb_samples) {
        std::unique_lock<std::mutex> lock1{lock};
        return av_audio_fifo_read(fifo, buffer, nb_samples);
    }

    bool empty() {
        std::unique_lock<std::mutex> lock1{lock};
        return av_audio_fifo_size(fifo) >= 0;
    }

    int size() {
        std::unique_lock<std::mutex> lock1{lock};
        return av_audio_fifo_size(fifo);
    }

    ~TSFifo() {
        av_audio_fifo_free(fifo);
    }
private:
    std::mutex lock{};
    AVAudioFifo* fifo;
    int max_samples;

};

struct convert_mp3_task : public libmediafy_task_t {
    
    convert_mp3_task(const std::string& url, const std::string& output_url) {
        // 1. Open the input file
        input_media = builder.set_url(url)
                            ->set_muxer(false)
                            ->create();

        if (!input_media) return;

        for (int i = 0; i < input_media->nb_streams(); i++) {
            auto stream = input_media->stream(i);
            if (stream->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
                std::vector<libmediafy::StreamDescription> descs;
                libmediafy::StreamDescription desc;
                desc.description.reset(avcodec_parameters_alloc(), libmediafy::AVCodecParamsDeleter());
                avcodec_parameters_copy(desc.description.get(), stream->codecpar);
                desc.description->codec_id = AV_CODEC_ID_MP3;
                desc.description->format = AV_SAMPLE_FMT_FLTP;
                descs.push_back(desc);

                auto ends_with = [](const std::string& text, const std::string& what) {
                    if (text.empty() || text.length() < what.length()) return false;
                    // Get an iterator that points to the last (what.length) chars in text
                    auto iter = text.end() - what.length();
                    auto iter2 = what.begin();
                    for (; iter != text.end() && iter2 != what.end(); iter++, iter2++) {
                        // We're gunning for this test failing
                        if ((*iter) != (*iter2)) return false;
                    }
                    // We made it!
                    return true;
                };

                std::string path = output_url;
                if (!ends_with(path, ".mp3")) {
                    // Append .mp3 to the end
                    path += ".mp3";
                }

                auto media = builder.set_muxer(true)
                                ->set_url(path)
                                ->set_stream_descriptions(descs)
                                ->set_output_format(nullptr)
                                ->create();

                if (!media) {
                    fprintf(stderr, "Couldn't create output media for file: %s\n", path.c_str());
                    continue;
                }

                istream = stream;

                output_media = media;

                // Get us mp3 encoder (if it's not an MP3 codec)
                if (stream->codecpar->format != AV_SAMPLE_FMT_FLTP) {
                    resampler.reset(swr_alloc_set_opts(nullptr, stream->codecpar->channel_layout, AV_SAMPLE_FMT_FLTP, stream->codecpar->sample_rate, stream->codecpar->channel_layout, (AVSampleFormat)stream->codecpar->format, stream->codecpar->sample_rate, 0, nullptr), [](SwrContext* ctx) {
                        swr_free(&ctx);
                    });
                }
                if (stream->codecpar->codec_id != AV_CODEC_ID_MP3) {
                    // Get us an encoder
                    encoder.reset(new libmediafy::Encoder(AV_CODEC_ID_MP3));
                    if (!encoder->initialize(desc.description.get())) {
                        fprintf(stderr, "Couldn't initialize the encoder!\n");
                        output_media.reset();
                        avpriv_io_delete(path.c_str());
                        return;
                    }

                    decoder.reset(new libmediafy::Decoder(stream->codecpar->codec_id, nullptr, true));
                    if (!decoder->initialize(stream->codecpar)) {
                        fprintf(stderr, "Unable to initialize the decoder!\n");
                        output_media.reset();
                        avpriv_io_delete(path.c_str());
                        return;
                    }
                    // We're always gon store 15 mins of audio
                    fifo.reset(new TSFifo(AV_SAMPLE_FMT_FLTP, stream->codecpar->channels, stream->codecpar->sample_rate * 60 * 5));
                }
                break;
            }
        }

        // 6. Set appropriate initialization flags and return
        cur_progress = 0;
        abort_req = false;

        fprintf(stderr, "Proc func!\n");
        th = std::thread(&convert_mp3_task::processing_func, this);
        th.detach();
        if (encoder) {
            encoding_th = std::thread(&convert_mp3_task::encoding_thread, this);
            encoding_th.detach();
        }
    }
    bool ready() override {
        std::unique_lock<std::mutex> lock200{lock};
        return output_media != nullptr;
    }
    int progress() override {
        std::unique_lock<std::mutex> lock200{lock};
        return cur_progress;
    }
    int resume() override {
        std::unique_lock<std::mutex> lock200{lock};
        if (abort_req) return -1;
        is_paused = false;
        state_cond.notify_one();
        return 0;
    }
    int pause() override {
        std::unique_lock<std::mutex> lock200{lock};
        if (abort_req) return -1;
        is_paused = true;
        return 0;
    }
    int abort() override {
        std::unique_lock<std::mutex> lock200{lock};
        if (abort_req) return -1;
        abort_req = true;
        return 0;
    }
    bool completed() override {
        std::unique_lock<std::mutex> lock200{lock};
        return is_completed;
    }
    bool paused() override {
        std::unique_lock<std::mutex> lock200{lock};
        return is_paused;
    }
    ~convert_mp3_task() override {
        abort();
    }

private:
    std::mutex lock{};
    libmediafy::MediaBuilder builder{};
    std::shared_ptr<libmediafy::Media> input_media;
    std::shared_ptr<libmediafy::Media> output_media;
    const AVStream* istream{nullptr};
    std::shared_ptr<SwrContext> resampler{};
    std::shared_ptr<libmediafy::Encoder> encoder{};
    std::shared_ptr<libmediafy::Decoder> decoder{};
    int cur_progress{0};
    bool is_completed{false};
    std::shared_ptr<TSFifo> fifo{};
    int64_t time{0};

    std::mutex state_lock{};
    std::atomic_bool is_paused{true};
    std::atomic_bool abort_req{true};
    std::condition_variable state_cond;
    std::thread th;
    std::thread encoding_th;
    std::atomic_bool decoding_done{false};
    std::mutex lock2{};

    void processing_func() {
        // This is where the logic happens
        using namespace std::chrono_literals;
        while (!abort_req) {
            std::unique_lock<std::mutex> lock222{state_lock};
            while (is_paused) {
                state_cond.wait_for(lock222, 100us);
                if (abort_req) return;
            }

            auto packet = input_media->read();

            if (!packet) {
                // This means that we've finished with the input media, we now need to flush the output media and we're done!
                if (decoder) {
                    auto frames = decoder->flush();
                    std::for_each(frames.begin(), frames.end(), [this](std::shared_ptr<AVFrame> frame) {
                        if (resampler) {
                            auto fr = av_frame_alloc();
                            fr->channels = frame->channels;
                            fr->channel_layout = frame->channel_layout;
                            fr->format = AV_SAMPLE_FMT_FLTP;
                            fr->sample_rate = frame->sample_rate;
                            fr->pts = frame->pts;

                            if (swr_convert_frame(resampler.get(), fr, frame.get()) < 0) {
                                fprintf(stderr, "Couldn't resample frame!\n");
                            }
                            frame.reset(fr, libmediafy::FrameDeleter());
                            fprintf(stderr, "Resampled frame!\n");
                        }
                        while (!fifo->write((void**)frame->data, frame->nb_samples)) {
                            std::this_thread::sleep_for(10us);
                        }
                    });
                    decoding_done = true;
                } else {
                    output_media->flush();
                    cur_progress = 100;
                    abort_req = true;
                    is_completed = true;
                }
                return;
            }

            if (packet->stream_index != istream->index) {
                continue;
            }

            if (istream->codecpar->codec_id == AV_CODEC_ID_MP3) {
                if (packet->pts != AV_NOPTS_VALUE) cur_progress = ((double)packet->pts / 1000.0) / (istream->duration * av_q2d(istream->time_base)) * 100.0;
                if (!output_media->write(packet.get())) {
                    fprintf(stderr, "Couldn't write packet to ostream!\n");
                }
                continue;
            }

            int res;
            auto frames = decoder->decode_packet(packet.get(), &res);
            std::for_each(frames.begin(), frames.end(), [this](std::shared_ptr<AVFrame> frame) {
                if (resampler) {
                    auto fr = av_frame_alloc();
                    fr->channels = frame->channels;
                    fr->channel_layout = frame->channel_layout;
                    fr->format = AV_SAMPLE_FMT_FLTP;
                    fr->sample_rate = frame->sample_rate;
                    fr->pts = frame->pts;

                    if (swr_convert_frame(resampler.get(), fr, frame.get()) < 0) {
                        fprintf(stderr, "Couldn't resample frame!\n");
                    }
                    frame.reset(fr, libmediafy::FrameDeleter());
                    fprintf(stderr, "Resampled frame!\n");
                }
                while (!fifo->write((void**)frame->data, frame->nb_samples)) {
                    std::this_thread::sleep_for(10us);
                }
            });
        }
    }

    void encoding_thread() {
        using namespace std::chrono_literals;
        while (!abort_req) {
            std::unique_lock<std::mutex> lockkk{lock2};
            while (is_paused) {
                state_cond.wait_for(lockkk, 100us);
                if (abort_req) return;
            }

            // If we're still decoding, always strive to get frame_size number of frames
            // Only when decoding is finished can we allow lesser frame numbers which will signal the end of the decoding

            bool flush{false};

            int frame_size = encoder->get_frame_size();
            if (decoding_done) {
                if (fifo->size() <= frame_size) {
                    frame_size = fifo->size();
                    flush = true;
                }
            } else {
                // Not enough samples!
                if (frame_size > fifo->size()) continue;
            }

            std::shared_ptr<AVFrame> frame{av_frame_alloc(), libmediafy::FrameDeleter()};
            frame->nb_samples = frame_size;
            frame->channel_layout = istream->codecpar->channel_layout;
            frame->sample_rate = istream->codecpar->sample_rate;
            frame->format = AV_SAMPLE_FMT_FLTP;
            if (av_frame_get_buffer(frame.get(), 0) < 0) {
                fprintf(stderr, "Not able to allocate buffer for frame!\n");
                continue;
            }
            fifo->read((void**)frame->data, frame->nb_samples);

            frame->pts = time;
            time += (int64_t)(((double)frame_size / istream->codecpar->sample_rate) * 1000);
            int res;
            auto packets = encoder->encode_frame(frame.get(), &res);
            std::for_each(packets.begin(), packets.end(), [this](std::shared_ptr<AVPacket> packet) {
                if (!output_media->write(packet.get())) {
                    fprintf(stderr, "Not writing to out!\n");
                }
            });

            double duration = 0;
            if (istream->duration == AV_NOPTS_VALUE) {
                duration = input_media->duration() / 1000.0;
            } else {
                duration = av_q2d(istream->time_base);
            }
            double num = ((double)frame->pts / 1000.0);
            cur_progress = (num / duration) * 100.0;

            // We're done, let's flush and gerrout!
            if (flush) {
                auto pkts = encoder->flush(&res);
                std::for_each(pkts.begin(), pkts.end(), [this](std::shared_ptr<AVPacket> pkt) {
                    if (!output_media->write(pkt.get())) {
                        fprintf(stderr, "Not writing to out!\n");
                    }
                });

                // Break outta the loop and set the done flag!
                output_media->flush();
                cur_progress = 100;
                abort_req = true;
                is_completed = true;
                break;
            }
        }
    }
};

LMDFY_EXPORT int libmediafy_convert_to_mp3(const char* url, const char* output, libmediafy_task* task) {
    libmediafy_task_t* task_t = new convert_mp3_task(url, output);
    if (!task_t->ready()) {
        delete task_t;
        return -1;
    }
    *task = (void*)task_t;
    return task_t->resume();
}

struct trim_media_task : public libmediafy_task_t {

    trim_media_task(const std::string& url, int64_t start, int64_t end, const std::string& output_file) : duration(end - start) {
        if (start >= end || end <= 0) return;
        // 1. Open the input file
        input_media = builder.set_url(url)
                ->set_muxer(false)
                ->create();

        if (!input_media) return;

        if (end >= input_media->duration()) return;

        if (start > 0 && !input_media->seek(start)) {
            fprintf(stderr, "Cannot seek to start position: %ld!\n", start);
            return;
        }

        std::vector<libmediafy::StreamDescription> descs;

        for (int i = 0; i < input_media->nb_streams(); i++) {
            auto stream = input_media->stream(i);

            if (stream->codecpar->codec_type == AVMEDIA_TYPE_UNKNOWN || stream->codecpar->codec_type == AVMEDIA_TYPE_DATA) {
                continue;
            }

            libmediafy::StreamDescription desc;
            desc.description.reset(avcodec_parameters_alloc(), libmediafy::AVCodecParamsDeleter());
            avcodec_parameters_copy(desc.description.get(), stream->codecpar);
            descs.push_back(desc);

            stream_mappings[i] = descs.size() - 1;
        }

        for (int i = 0; i < input_media->nb_streams(); i++) {
            starts.push_back(-1);
        }

        auto ends_with = [](const std::string& text, const std::string& what) {
            if (text.empty() || text.length() < what.length()) return false;
            // Get an iterator that points to the last (what.length) chars in text
            auto iter = text.end() - what.length();
            auto iter2 = what.begin();
            for (; iter != text.end() && iter2 != what.end(); iter++, iter2++) {
                // We're gunning for this test failing
                if ((*iter) != (*iter2)) return false;
            }
            // We made it!
            return true;
        };

        auto pos = url.find_last_of('.');
        if (pos == std::string::npos) {
            fprintf(stderr, "This file doesn't have an extension! Bailing...\n");
            return;
        }
        auto ext = url.substr(pos);

        auto file = output_file;

        if (!ends_with(output_file, ext)) {
            file += ext;
        }

        output = builder.set_muxer(true)
                ->set_url(file)
                ->set_stream_descriptions(descs)
                ->set_output_format(nullptr)
                ->create();

        if (!output) {
            fprintf(stderr, "Couldn't create output media for ext: %s\n", ext.c_str());
            return;
        }

        // 6. Set appropriate initialization flags and return
        is_ready = true;
        cur_progress = 0;
        abort_req = false;

        th = std::thread(&trim_media_task::processing_func, this);
        th.detach();
    }
    bool ready() override {
        std::unique_lock<std::mutex> lock2{lock};
        return is_ready;
    }
    int progress() override {
        std::unique_lock<std::mutex> lock2{lock};
        return cur_progress;
    }
    int resume() override {
        std::unique_lock<std::mutex> lock2{lock};
        if (abort_req) return -1;
        is_paused = false;
        state_cond.notify_one();
        return 0;
    }
    int pause() override {
        std::unique_lock<std::mutex> lock2{lock};
        if (abort_req) return -1;
        is_paused = true;
        return 0;
    }
    int abort() override {
        std::unique_lock<std::mutex> lock2{lock};
        if (abort_req) return -1;
        abort_req = true;
        return 0;
    }
    bool completed() override {
        std::unique_lock<std::mutex> lock2{lock};
        return is_completed;
    }
    bool paused() override {
        std::unique_lock<std::mutex> lock2{lock};
        return is_paused;
    }
    ~trim_media_task() override {
        abort();
    }

private:
    std::mutex lock{};
    libmediafy::MediaBuilder builder{};
    std::shared_ptr<libmediafy::Media> input_media;
    std::shared_ptr<libmediafy::Media> output;
    bool is_ready{false};
    int cur_progress{-1};
    bool is_completed{false};
    int64_t duration;

    std::map<int, int> stream_mappings; // Maps the input streams to the output streams

    std::mutex state_lock{};
    std::atomic_bool is_paused{true};
    std::atomic_bool abort_req{true};
    std::condition_variable state_cond;
    std::thread th;
    std::vector<int64_t> starts;

    void processing_func() {
        // This is where the logic happens
        using namespace std::chrono_literals;
        while (!abort_req) {
            std::unique_lock<std::mutex> lock2{state_lock};
            while (is_paused) {
                state_cond.wait_for(lock2, 100us);
                if (abort_req) return;
            }

            // The process is,
            // Read and write a frame, write the progress and then circle back on the loop
            // This is so that we can reliably support the pause-abort mechanism

            auto packet = input_media->read();
            if (!packet) {
                // This means that we've finished with the input media, we now need to flush the output media and we're done!
                output->flush();
                cur_progress = 100;
                abort_req = true;
                is_completed = true;
                break;
            }

            // We don't have this index in our stream mappings
            if (stream_mappings.find(packet->stream_index) == stream_mappings.end()) continue;

            // Pick a stream to base the timing off of
            if (packet->pts != AV_NOPTS_VALUE) {
                if (starts[packet->stream_index] < 0) {
                    starts[packet->stream_index] = packet->pts;
                }
                packet->pts -= starts[packet->stream_index];
                packet->dts = packet->pts;
            }

            if (packet->pts <= duration) {
                packet->stream_index = stream_mappings[packet->stream_index];
                if (!output->write(packet.get())) {
                    fprintf(stderr, "Couldn't write output to the muxer!\n");
                }
            } else {
                output->flush();
                cur_progress = 100;
                abort_req = true;
                is_completed = true;
                break;
            }
        }
        end:
        is_completed = true;
    }
};

LMDFY_EXPORT int libmediafy_trim(const char* url, int64_t start, int64_t end, const char* output, libmediafy_task* task) {
    libmediafy_task_t* task_t = new trim_media_task(url, start, end, output);
    if (!task_t->ready()) {
        delete task_t;
        return -1;
    }
    *task = (void*)task_t;
    return task_t->resume();
}

struct merge_task : public libmediafy_task_t {
    merge_task(const char** urls, int count, const char* output) {
    }
    int progress() override {
        return _progress;
    }
    int resume() override {
        _paused = false;
        return 0;
    }
    bool ready() override {
        return _ready;
    }
    int pause() override {
        _paused = true;
        return 0;
    }
    int abort() override {
        _abort = true;
        return 0;
    }
    bool completed() override {
        return _completed;
    }
    bool paused() override {
        return _paused;
    }
    ~merge_task() override = default;

private:
    std::vector<std::shared_ptr<libmediafy::Media>> _inputs;
    std::shared_ptr<libmediafy::Media> _output_media;
    bool _ready{false};
    bool _completed{false};
    bool _paused;
    bool _abort{true};
    int _progress;

    void _process() {
    }
};

LMDFY_EXPORT int libmediafy_merge(const char** urls, int count, const char* output_url, libmediafy_task* task) {
    return 0;
}

}
