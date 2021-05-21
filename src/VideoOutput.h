//
// Created by adebayo on 4/4/21.
//

#ifndef LIBMEDIAFYANDROID_VIDEORENDERER_H
#define LIBMEDIAFYANDROID_VIDEORENDERER_H

extern "C" {
#include <libavutil/frame.h>
#include <libavcodec/jni.h>
}

#ifdef ANDROID
#include <android/native_window_jni.h>
#include "ThreadExecutor.h"
#include <EGL/egl.h>
#endif

#include <string>
#include <memory>

namespace libmediafy {
    class VideoOutput {
    public:
        VideoOutput();
        VideoOutput(const VideoOutput&) = delete;

        void set_output_surface(void* surface);

        void set_input_format(int format) {
            this->format = format;
        }

        void set_subtitles_view(void* sub_view);

        /// Render the frame
        void render(std::shared_ptr<AVFrame> frame);

        /// Draw some text on top of the video.
        void draw_text(const std::string& text);

        /// Dispose of the resources we created cause we need them no more
        void dispose();

        ~VideoOutput();

#ifdef ANDROID
        void* get_input_surface();
        void on_frame_available();
#endif

    private:
        int format;
#ifdef ANDROID
        jobject renderer_java;
        jmethodID set_texture_id;
        jmethodID reset_texture_id;
        jmethodID update_image;
        jmethodID get_surface;
        jmethodID set_subtitle_textview;
        jmethodID render_text;
        ANativeWindow* output_surface;
        ThreadExecutor gl_executor;
        std::mutex output_surface_lock{};
        bool egl_initialized;

        EGLDisplay display;
        EGLSurface eglOutputSurface;
        EGLContext context;
        EGLConfig config;

        int mediacodec_program;
        int yuv_program;
        uint texture;
        uint yuv_textures[3];

        std::shared_ptr<AVFrame> last_frame;
        
        void initEGL();
        void destroyEGL();
        void render_texture(std::shared_ptr<AVFrame> frame);
        
        JNIEnv* get_env() {
            JavaVM* vm = (JavaVM*)av_jni_get_java_vm(nullptr);
            JNIEnv* env{nullptr};
            int ret = vm->GetEnv((void**)&env, JNI_VERSION_1_6);
            if (ret == JNI_EDETACHED) {
                vm->AttachCurrentThread(&env, nullptr);
            }
            return env;
        }
#endif
    };
}


#endif //LIBMEDIAFYANDROID_VIDEORENDERER_H
