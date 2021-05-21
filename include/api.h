#pragma once
#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

/// This is the libmediafy API that gets exposed to the outside world

/// This class will contain the functions used to communicate with dart
/// It is implemented in the lib.cpp file.
typedef void *libmediafy_player;

typedef struct libmediafy_image {
    void* buffer;
    int32_t size;
    void* _internal_obj; // Refers to the internal AVPacket. Touch it if you want to leak memory
} libmediafy_image;

/// Audio formats
typedef enum {
    libmediafy_afmt_s16,
    libmediafy_afmt_f32,
} libmediafy_afmt;

typedef struct libmediafy_aframe {
    libmediafy_afmt format;
    int channels;
    int nb_samples;
    void* buffer;
    int32_t size;
    void* _internal_obj; // Refers to the internal AVFrame
} libmediafy_aframe;

typedef void* libmediafy_task;


#define LMDFY_EXPORT __attribute((visibility ("default")))

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
//                            MEDIA PLAYER API                               //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

/// Create a new media player instance
LMDFY_EXPORT libmediafy_player libmediafy_player_new();


/// Load media for the player to play. Returns 0 if operation was successful and -1 otherwise
LMDFY_EXPORT int libmediafy_player_set_data_source(libmediafy_player player, const char* url);


/// Whether the media player has a playable media. Returns 1 if true and 0 otherwise
LMDFY_EXPORT int libmediafy_player_has_media(libmediafy_player player);


/// Free the media player
LMDFY_EXPORT int libmediafy_player_delete(libmediafy_player* player);


/// Start playing media
LMDFY_EXPORT int libmediafy_player_play(libmediafy_player player);


/// Pause media playback
LMDFY_EXPORT int libmediafy_player_pause(libmediafy_player player);


/// Stop media playback
LMDFY_EXPORT int libmediafy_player_stop(libmediafy_player player);


/// Whether a media is playing
LMDFY_EXPORT int libmediafy_player_is_playing(libmediafy_player player);


/// Seek to a specified position
LMDFY_EXPORT int libmediafy_player_seek(libmediafy_player player, double pos_secs);


/// Get the current volume of the media player (1.0 is default and 4.0 is max)
LMDFY_EXPORT float libmediafy_player_get_volume(libmediafy_player player);


/// Set the current volume of the media player
LMDFY_EXPORT int libmediafy_player_set_volume(libmediafy_player player, float value);


/// Returns the duration of the media (in seconds) currently playing. Returns zero is no media is playing or this media player is invalid
LMDFY_EXPORT double libmediafy_player_get_duration(libmediafy_player player);


/// Returns the playing time of the current media
LMDFY_EXPORT double libmediafy_player_get_time(libmediafy_player player);


/// Returns the number of bands for the audio equalizer
LMDFY_EXPORT int libmediafy_player_get_equalizer_bands(libmediafy_player player);


/// Enables the equalizer
LMDFY_EXPORT int libmediafy_player_enable_equalizer(libmediafy_player player);


/// Disables the equalizer
LMDFY_EXPORT int libmediafy_player_disable_equalizer(libmediafy_player player);


/// Returns whether the equalizer is enabled
LMDFY_EXPORT int libmediafy_player_is_equalizer_enabled(libmediafy_player player);


/// Returns the equalizer frequency at the specified index (in decibels)
LMDFY_EXPORT float libmediafy_player_get_equalizer_frequency(libmediafy_player player, int index);


/// Sets the equalizer frequency at the specified index (in decibels)
LMDFY_EXPORT int libmediafy_player_set_equalizer_frequency(libmediafy_player player, int index, float value);


/// Returns the number of equalizer presets available internally
LMDFY_EXPORT int libmediafy_player_get_num_equalizer_presets(libmediafy_player player);


/// Returns the name of a preset at the specified index
LMDFY_EXPORT const char* libmediafy_player_get_equalizer_preset_name(libmediafy_player player, int preset_index);


/// Returns the preset frequency at the specified index
LMDFY_EXPORT float libmediafy_player_get_equalizer_preset_frequency(libmediafy_player player, int preset_index, int freq_index);


/// Returns the preamp value for the equalizer
LMDFY_EXPORT float libmediafy_player_get_equalizer_preamp(libmediafy_player player);


/// Sets the preamp value for the equalizer
LMDFY_EXPORT int libmediafy_player_set_equalizer_preamp(libmediafy_player player, float preamp);


/// Sets the preset to use for the equalizer
LMDFY_EXPORT int libmediafy_player_set_equalizer_preset(libmediafy_player player, int preset_index);


/// Loads the album art for the currently playing media
LMDFY_EXPORT int libmediafy_player_get_album_art(libmediafy_player player, libmediafy_image **out_frame);


/// Loads a thumbnail of the specified size and stores the result in out_buffer. The size is stored in out_size. The buffer must be freed using libmediafy_free_buffer when the user is done with it.
/// If the thumbnail cannot be loaded, -1 is returned and out_buffer and out_size will be untouched. Returns 0 if the task was successful
LMDFY_EXPORT int libmediafy_load_thumbnail(const char* path, int width, int height, libmediafy_image **out_frame);


/// Loads a full-size album art for the specified media.
LMDFY_EXPORT int libmediafy_load_album_art(const char *path, libmediafy_image **out_frame);


/// Loads a video frame from the specified position. The time units is in milliseconds
LMDFY_EXPORT int libmediafy_load_video_frame(const char *path, uint64_t pos_millis, libmediafy_image **out_frame);


/// Requests to pause this task. Note that this is just a request and that the implementation can choose to ignore it in cases where it deems it's not appropriate to perform such an action.
/// You can determine whether an task has been paused by checking libmediafy_task_is_paused
LMDFY_EXPORT int libmediafy_task_pause(libmediafy_task task);


/// Whether an task has been paused. Returns 1 if true and 0 otherwise
LMDFY_EXPORT int libmediafy_task_is_paused(libmediafy_task task);


/// Resume a paused task
LMDFY_EXPORT int libmediafy_task_resume(libmediafy_task task);


/// Requests to abort the task. Note that this is a request to the underlying implementation to abort the task and the implementation will decide whether or not to honour this request depending on constraints
LMDFY_EXPORT int libmediafy_task_abort(libmediafy_task task);


/// Returns the tasks's progress. If the task has failed and is no more running, it returns -1 to communicate that fact. Otherwise, it returns progress within the range of 0 - 100.
LMDFY_EXPORT int libmediafy_task_progress(libmediafy_task task);


/// Whether the task has completed. Returns 1 if true and 0 otherwise. Returns -1 if the task completed with failure
LMDFY_EXPORT int libmediafy_task_is_complete(libmediafy_task task);


/// Destroy the task. If it has completed, the memory is freed. If the task is still running, it is aborted and the memory is freed. This function will block till the task has been aborted
LMDFY_EXPORT int libmediafy_task_free(libmediafy_task *task);


/// Free an allocated video frame
LMDFY_EXPORT int libmediafy_free_image(libmediafy_image **frame);


/// Free an allocated audio frame
LMDFY_EXPORT int libmediafy_free_aframe(libmediafy_aframe **frame);


/// Splits the media into their individual streams. Streams are saved with whatever codecs they were encoded with and a suitable container. Other streams (such as subtitles) are ignored. dir must be writable and this is where the files will be saved. The output files will have the name of the input file plus a number at the end indicating (which is the stream index) and a new file extension corresponding to the container. Streams for which codecs cannot be found will be ignored and num_streams will contain the number of streams that will be processed.
/// This function spawns the task asynchronously, and the progress can be tracked using the libmediafy_task API. The task can also be managed using this pointer. The task will be automatically started on successful configuration
/// Returns 0 if task has started and -1 otherwise
LMDFY_EXPORT int libmediafy_split_media(const char *url, const char *output_dir, int *num_streams, libmediafy_task *task);


/// Convert the media to mp3. If the media is an mp3 media file, this will just copy the input to the output without re-encoding. Otherwise, it will take an audio stream from the media, decode the samples, and encode them to mp3.
/// This function spawns the task asynchronously, and the progress can be tracked using the libmediafy_task API. The task is started upon successful configuration and 0 is returned. If an error occurs, you get a negative integer and no task is started
LMDFY_EXPORT int libmediafy_convert_to_mp3(const char* url, const char* output, libmediafy_task* task);


/// Trims the media. Don't matter whether it's audio or video, it's gon trim it!!!
/// Specify the timing in milliseconds. If the range is not valid, the operation will fail. Note that whatever file extension you pass in as part of the output media file name is going to be ignored and will use the format of the original media it's trimming and append the file extension of the source url to the output. This is so that the function will not have to re-encode the data and can just copy the compressed frames to the output
LMDFY_EXPORT int libmediafy_trim(const char* url, int64_t start, int64_t end, const char* output, libmediafy_task* task);


/// Merges two or more media files. If you try to merge 0 or 1 file, it's just going to return a negative integer and terminate. It's going to merge the media files in the order specified in the urls array. If the media contains both audio and video files, they will be merged and this function will perform re-encoding of the data. If the formats and codecs are the same however, the function will just copy all the data into a new container. Returns a task you can use to track progress of the operation.
// Note that the output url will be changed
LMDFY_EXPORT int libmediafy_merge(const char** urls, int count, const char* output_url, libmediafy_task* task);

#ifdef __cplusplus
}
#endif
