//
// Created by adebayo on 4/3/21.
//

#include "MediaPlayer.h"

extern "C" {
#include <libavutil/time.h>
#include <libavcodec/mediacodec.h>
}

#include "Equalizer.h"
#include "StereoWidener.h"

using namespace std::chrono_literals;

libmediafy::MediaPlayer::MediaPlayer() {
    equalizer.reset(new Equalizer(48000));
    volume.reset(new Gain());
}

bool libmediafy::MediaPlayer::set_data_source(const char *url) {
    if (!url) return false;
    release();
    MediaBuilder builder{};
    current_media = builder.set_url(url)->create();
    if (!current_media) return false;
    this->url = url;
    for (int i = 0; i < current_media->nb_streams(); i++) {
        auto* stream = current_media->stream(i);
        AVCodecID codec_id = stream->codecpar->codec_id;
        if (stream->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            if (stream->disposition & AV_DISPOSITION_ATTACHED_PIC) continue;
            if (!video_decoder || video_stream_index < 0) {
                #ifdef ANDROID
                video_decoder = new Decoder(codec_id, vout_renderer.get_input_surface());
                #else
                video_decoder = new Decoder(codec_id, nullptr);
                #endif
                if (!video_decoder->initialize(stream->codecpar)) {
                    printf("Couldn't initialize video decoder!");
                    delete video_decoder;
                    video_decoder = nullptr;
                } else {
                    video_stream_index = i;
                    video_output_thread = std::thread(&MediaPlayer::vout_thread, this);
                    vout_renderer.set_input_format((AVPixelFormat)stream->codecpar->format);
                    pthread_setname_np(video_output_thread.native_handle(), "LMDFY VOUT");
                }
            }
        } else if (stream->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
            if (!audio_decoder) {
                audio_decoder = new Decoder(codec_id, nullptr);
                if (!audio_decoder->initialize(stream->codecpar)) {
                    delete audio_decoder;
                    audio_decoder = nullptr;
                } else {
                    equalizer->reset(stream->codecpar->sample_rate);
                    compressor = new ACompressor(stream->time_base, stream->codecpar->sample_rate, stream->codecpar->format, stream->codecpar->channel_layout);
                    audio_stream_index = i;
                    audio_output_builder.set_channels(stream->codecpar->channels)
                        .set_audio_callback_handler(this)
                        .set_sample_format(AV_SAMPLE_FMT_FLT)
                        .set_sample_rate(stream->codecpar->sample_rate);
                    audio_output = std::move(audio_output_builder.create());
                    resampler = swr_alloc_set_opts(nullptr, stream->codecpar->channel_layout, AV_SAMPLE_FMT_FLT, stream->codecpar->sample_rate, stream->codecpar->channel_layout, (AVSampleFormat)stream->codecpar->format, stream->codecpar->sample_rate, 0, nullptr);
                }
            }
        } else if (stream->codecpar->codec_type == AVMEDIA_TYPE_SUBTITLE) {
            subtitlesManager.flush();
            subtitlesManager.set_time_base(av_q2d(stream->time_base));
            subtitles_stream_index = i;
        }
    }

    demuxing = true;
    demuxer_thread = std::thread(&MediaPlayer::demux_thread, this);
    pthread_setname_np(demuxer_thread.native_handle(), "LMDFY DEMUX");

    if (audio_stream_index >= 0) {
        audio_dec_th = std::thread(&MediaPlayer::audio_decoder_thread, this);
        pthread_setname_np(audio_dec_th.native_handle(), "LMDFY ADEC");
    }

    if (video_stream_index >= 0) {
        video_dec_th = std::thread(&MediaPlayer::video_decoder_thread, this);
        pthread_setname_np(video_dec_th.native_handle(), "LMDFY VDEC");
    }

    finished_pb = false;
    initialized = true;

    printf("Finished initialize!\n");
    return true;
}

void libmediafy::MediaPlayer::play() {
    std::unique_lock<std::mutex> lock{global_lock};
    if (!has_media()) {
        printf("No media to play!\n");
        return;
    }
    if (!playing) {
        playing = true;
        printf("Playing now...\n");
        audio_frames_cond.notify_all();
        video_frames_cond.notify_all();
        audio_packets_cond.notify_all();
        video_packets_cond.notify_all();
        playing_cond.notify_all();
        demux_cond.notify_one();
        if (audio_output) audio_output->start();
    }
}

void libmediafy::MediaPlayer::pause() {
    std::unique_lock<std::mutex> lock{global_lock};
    if (!has_media()) return;
    if (playing) {
        if (audio_output) audio_output->stop();
        playing = false;
        printf("Paused");
        demux_cond.notify_one();
    }
}

void libmediafy::MediaPlayer::stop() {
    std::unique_lock<std::mutex> lock{global_lock};
    if (!has_media()) return;
    pause();
    printf("Stopped");
}

void libmediafy::MediaPlayer::seek(double position) {
    std::unique_lock<std::mutex> lock{global_lock};
    if (!has_media() || position < 0.0 || position > (current_media->duration() / 1000.0)) return;
    seek_requested = true;
    seek_position.store(position);

    demux_cond.notify_one();
    playing_cond.notify_all();
}

void libmediafy::MediaPlayer::release() {
    std::unique_lock<std::mutex> lock{global_lock};
    std::unique_lock<std::mutex> lock2{audio_output_lock};
    printf("Releasing media\n");

    if (!has_media()) return;

    if (audio_output) audio_output->close();

    if (url) {
        printf("Previous URL was: %s", url);
    }
    url = nullptr;
    playing = false;
    seek_requested = false;
    seek_position = 0;
    current_time = 0;
    seek_update_frame = false;
    playing_cond.notify_all();
    if (video_output_thread.joinable()) video_output_thread.join();

    printf("Stopping demux operations...");
    if (demuxing) {
        demuxing = false;
        demux_cond.notify_one();
        video_frames_cond.notify_all();
        audio_frames_cond.notify_all();
        audio_packets_cond.notify_all();
        video_packets_cond.notify_all();
        if (demuxer_thread.joinable()) demuxer_thread.join();
    }

    printf("Stopping the decoders...");
    if (video_dec_th.joinable()) video_dec_th.join();
    if (audio_dec_th.joinable()) audio_dec_th.join();

    audio_stream_index = -1;
    video_stream_index = -1;
    subtitles_stream_index = -1;

    if (resampler) swr_free(&resampler);

    clear_buffers();

    vout_renderer.dispose();

    if (audio_decoder) {
        delete audio_decoder;
        audio_decoder = nullptr;
    }
    if (video_decoder) {
        delete video_decoder;
        video_decoder = nullptr;
    }

    current_time = 0;

    subtitlesManager.flush();

    delete compressor;

    printf("Release finished!");
    finished_pb = true;
    reported_pb_finished = false;
}

void libmediafy::MediaPlayer::demux_thread() {
    while (demuxing) {
        std::unique_lock<std::mutex> lock{demux_lock};
        bool update_time{false};
        if (seek_requested) {
            printf("Seek requested! Seeking to: %f", seek_position.load());
            // Perform the seek
            reported_pb_finished = false;
            current_media->seek(static_cast<int64_t>(seek_position * 1000));
            update_time = true;
            finished_pb = false;
            clear_buffers();
            seek_requested = false;
            seek_update_frame = true;
        }

        auto packet = current_media->read();
        if (!packet) {
            demux_cond.notify_all();
            demux_cond.wait_for(lock, 100us);
            continue;
        }
        if (update_time) current_time = packet->pts / 1000;
        if (packet->stream_index == video_stream_index) {
            while (!video_packets.enqueue(packet)) {
                if (!demuxing || !has_media()) {
                    break;
                }
                video_packets_cond.notify_all();
                video_packets_cond.wait_for(lock, 100us);
            }
        } else if (packet->stream_index == audio_stream_index) {
            while (!audio_packets.enqueue(packet)) {
                if (!demuxing) {
                    break;
                }
                audio_packets_cond.notify_all();
                audio_packets_cond.wait_for(lock, 100us);
            }
        } else if (packet->stream_index == subtitles_stream_index) {
            subtitlesManager.queue_packet(packet);
        }
    }

    printf("Demux thread finished!");
}

void libmediafy::MediaPlayer::vout_thread() {
    printf("Vout thread!");
    Cue cue{};

    auto render_frame = [&](std::shared_ptr<AVFrame> frame) {
        if (frame->format == AV_PIX_FMT_MEDIACODEC) {
            AVMediaCodecBuffer* buffer = (AVMediaCodecBuffer*) frame->data[3];
            av_mediacodec_release_buffer(buffer, 1);
        } else {
            vout_renderer.render(frame);
        }
    };

    while (url) {
        std::unique_lock<std::mutex> lock{vout_thread_lock};
        if (!playing) {
            playing_cond.notify_all();
            playing_cond.wait_for(lock, 100us);

            if (seek_update_frame) {
                std::shared_ptr<AVFrame> frame{nullptr};
                if (!video_frames.empty()) {
                    frame = video_frames.dequeue();
                    video_frames_cond.notify_all();
                }
                if (frame) {
                    double last = last_video_pts;
                    last_video_pts = frame->pts / 1000;
                    printf("PAUSED: Last video pts: %f, Current pts: %f", last, last_video_pts.load());
                    report_time_changed(last_video_pts);
                    render_frame(frame);
                    seek_update_frame = false;
                }
            }
            continue;
        }

        std::shared_ptr<AVFrame> frame{nullptr};
        if (video_frames.empty()) {
            video_frames_cond.notify_all();
            video_frames_cond.wait_for(lock, 100us);
            if (!url) {
                goto end;
            }
            continue;
        }
        frame = video_frames.dequeue();
        video_frames_cond.notify_all();

        if (frame) {
            if (frame->pts != AV_NOPTS_VALUE) {
                double pts = frame->pts / 1000.0;
                double diff = pts - last_video_pts;
                double f_rate = av_q2d(current_media->stream(video_stream_index)->r_frame_rate);

                // This is useful for when seeking, cause we might get non-contiguous pts data
                if (diff > (1 / f_rate)) diff = 1 / f_rate;
                bool drop{false};
                last_video_pts = pts;

                double time_diff = (av_gettime_relative() - last_video_time) / 1000000.0;
                if (time_diff > diff) {
                    diff = 0;
//                    printf("Video behind its timer: %f", time_diff - diff);
                } else {
                    diff -= time_diff;
                }

                auto last_audio_pts = this->last_audio_pts.load();

                if (audio_decoder) {
                    double pts_diff = fabs(last_video_pts - last_audio_pts);
                    if (pts_diff > 0.10) {
                        if (last_audio_pts > last_video_pts) {
                            drop = true;
                            diff = pts_diff;
                        } else {
                            diff += pts_diff;
                        }
                    }
                } else {
                    report_time_changed(pts);
                }

                if (diff > 0.0 && !drop && diff < 0.6) {
                    printf("Sleeping for %f seconds. Audio time: %f, Video time: %f", diff, last_audio_pts, last_video_pts.load());
                    av_usleep(static_cast<unsigned int>(diff * 1000000));
                }

                if (!drop) {
                    render_frame(frame);
                    if (subtitlesManager.get_cue(last_video_pts, &cue)) {
                        vout_renderer.draw_text(cue.data);
                    } else {
                        vout_renderer.draw_text("");
                    }
                } else {
//                    printf("Dropping frame with %f diff. Audio time: %f, Video time: %f", diff, last_audio_pts, last_video_pts.load());
                    if (frame->format == AV_PIX_FMT_MEDIACODEC) {
                        AVMediaCodecBuffer *buffer = (AVMediaCodecBuffer *) frame->data[3];
                        av_mediacodec_release_buffer(buffer, 0);
                    }
                }
            }
        }
        last_video_time = av_gettime_relative();
    }
end:
    printf("VOUT thread finished!");
}

void libmediafy::MediaPlayer::report_time_changed(double time) {
    int64_t duration = current_media->duration();
    if (duration <= 0) return;
    double dur_d = duration / 1000.0;
    if (finished_pb && time >= dur_d) return;
    current_time = time;
    finished_pb = (int64_t)time >= (int64_t)dur_d;
}

void libmediafy::MediaPlayer::video_decoder_thread() {
    printf("Video dec in service!");
    while (demuxing) {
        std::unique_lock<std::mutex> lock{video_dec_lock};
        std::shared_ptr<AVPacket> packet{nullptr};
        while (video_packets.empty()) {
            demux_cond.notify_one();
            video_packets_cond.notify_all();
            video_packets_cond.wait_for(lock, 100us);
            if (!demuxing) {
                goto end;
            }
        }
        packet = video_packets.dequeue();
        demux_cond.notify_one();
        video_packets_cond.notify_all();
        while (packet) {
            if (!demuxing) break;
            int res;
            auto frames = video_decoder->decode_packet(packet.get(), &res);

            if (res == 0 || res != AVERROR(EAGAIN)) {
                packet.reset();
            }

            for (int i = 0; i < frames.size(); i++) {
                while (!video_frames.enqueue(frames[i])) {
                    if (!demuxing) {
                        goto abort_loop;
                    }
                    video_frames_cond.notify_all();
                    video_frames_cond.wait_for(lock, 100us);
                }
                video_frames_cond.notify_all();
            }
        abort_loop:
            continue;
        }
    }
end:
    printf("Video dec thread finished!");
}

void libmediafy::MediaPlayer::audio_decoder_thread() {
    printf("Audio decoder thread...\n");
    while (demuxing) {
        std::unique_lock<std::mutex> lock{audio_dec_lock};
        std::shared_ptr<AVPacket> packet{nullptr};
        while (audio_packets.empty()) {
            demux_cond.notify_all();
            audio_packets_cond.notify_all();
            audio_packets_cond.wait_for(lock, 100us);
            if (!demuxing) {
                goto end;
            }
        }
        packet = audio_packets.dequeue();
        audio_packets_cond.notify_all();
        demux_cond.notify_one();
        while (packet) {
            if (!demuxing) break;

            int res;
            auto frames = audio_decoder->decode_packet(packet.get(), &res);

            if (res == 0 || res != AVERROR(EAGAIN)) {
                packet.reset();
            }

            for (int i = 0; i < frames.size(); i++) {
                // Resample the audio
                std::shared_ptr<AVFrame> resampled_frame{av_frame_alloc(), FrameDeleter()};
                resampled_frame->pts = frames[i]->pts;
                resampled_frame->sample_rate = frames[i]->sample_rate;
                resampled_frame->format = AV_SAMPLE_FMT_FLT;
                resampled_frame->channels = frames[i]->channels;
                resampled_frame->channel_layout = frames[i]->channel_layout;

                if (swr_convert_frame(resampler, resampled_frame.get(), frames[i].get())) {
                    printf("Couldn't convert frame!");
                    continue;
                }
                filter_frame(resampled_frame);

                while (!audio_frames.enqueue(resampled_frame)) {
                    if (!demuxing || seek_requested) {
                        goto abort_loop;
                    }
                    audio_frames_cond.notify_all();
                    audio_frames_cond.wait_for(lock, 100us);
                }
            }
        abort_loop: continue;
        }
    }
end:
    printf("Audio dec thread finished!");
}

double libmediafy::MediaPlayer::get_current_time() {
    if (seek_requested) return seek_position;
    return current_time;
}

bool libmediafy::MediaPlayer::get_buffer(int num_frames, unsigned int bytes_per_frame, void* buffer) {
    if (!audio_output_lock.try_lock()) return false;
    uint8_t* buf = (uint8_t*)buffer;
    while (num_frames > 0) {
        std::shared_ptr<AVFrame> frame{nullptr};
        if (audio_frames.empty()) {
            break;
        }

        frame = audio_frames.peek();

        // Set current time
        report_time_changed(frame->pts / 1000);
        last_audio_pts = frame->pts / 1000.0;

        int num_samples = std::min(frame->nb_samples, num_frames);
        int bytes = num_samples * bytes_per_frame;
        memcpy(buf, frame->data[0], (size_t)bytes);

        buf += bytes;
        num_frames -= num_samples;
        frame->nb_samples -= num_samples;
        frame->data[0] += bytes;

        if (frame->nb_samples <= 0) {
            audio_frames.remove_front();
            audio_frames_cond.notify_all();
        }
    }
    audio_output_lock.unlock();
    return true;
}

void libmediafy::MediaPlayer::filter_frame(std::shared_ptr<AVFrame> frame) {
    if (volume) {
        if (!volume->filter((float*)frame->data[0], frame->nb_samples, frame->channels)) {
            printf("Volume filtering failed!");
        }
    }

    if (equalizer && equalizer_active) {
        if (!equalizer->filter((float*)frame->data[0], frame->nb_samples, frame->channels)) {
            printf("Equalizer filtering failed!");
        }
    }
}
