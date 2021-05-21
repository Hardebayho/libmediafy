//
// Created by adebayo on 4/4/21.
//

#include "VideoOutput.h"

#ifdef ANDROID

extern "C" {
#include <libavcodec/mediacodec.h>
#include <libavutil/hwcontext_mediacodec.h>
}

#include <GLES3/gl3.h>
#include <GLES2/gl2ext.h>
#include <android/log.h>

libmediafy::VideoOutput::VideoOutput() {
    JNIEnv* env = get_env();
    jclass clasz = env->FindClass("tech/smallwonder/libmediafyandroid/VideoOutput");
    jmethodID constructor = env->GetMethodID(clasz, "<init>", "(J)V");
    renderer_java = env->NewObject(clasz, constructor, reinterpret_cast<jlong>(this));
    renderer_java = env->NewGlobalRef(renderer_java);

    set_texture_id = env->GetMethodID(clasz, "setTextureID", "(I)V");
    reset_texture_id = env->GetMethodID(clasz, "resetTextureID", "()V");
    update_image = env->GetMethodID(clasz, "updateImage", "()V");
    get_surface = env->GetMethodID(clasz, "getSurface", "()Landroid/view/Surface;");
    set_subtitle_textview = env->GetMethodID(clasz, "setSubtitleTextView", "(Landroid/widget/TextView;)V");
    render_text = env->GetMethodID(clasz, "renderText", "(Ljava/lang/String;)V");
    env->DeleteLocalRef(clasz);
}

libmediafy::VideoOutput::~VideoOutput() {
    JNIEnv* env = get_env();
    env->DeleteGlobalRef(renderer_java);
    gl_executor.queue_execute([&](){
        destroyEGL();
    });
}

void* libmediafy::VideoOutput::get_input_surface() {
#ifdef ANDROID
    return get_env()->CallObjectMethod(renderer_java, get_surface);
#else
    return nullptr;
#endif
}

void libmediafy::VideoOutput::set_output_surface(void* output_surface) {
#ifdef ANDROID
    std::unique_lock<std::mutex> lock{output_surface_lock};
    if (!output_surface) {
        gl_executor.queue_execute([&]() {
            destroyEGL();
            if (this->output_surface) {
                ANativeWindow_release(this->output_surface);
            }
            this->output_surface = nullptr;
        });
    } else {
        if (this->output_surface) ANativeWindow_release(this->output_surface);
        this->output_surface = ANativeWindow_fromSurface(get_env(), (jobject)output_surface);
        gl_executor.queue_execute([&]() {
            destroyEGL();
            initEGL();
            render_texture(nullptr);
        });
    }
#endif
}

void libmediafy::VideoOutput::render(std::shared_ptr<AVFrame> frame) {
    if (frame->format != format) {
        printf("Format is not the same! Frame's format: %d, format: %d", frame->format, format);
        return;
    }
    switch (frame->format) {
        case AV_PIX_FMT_YUV420P:
            gl_executor.queue_execute([&, frame]() {
                render_texture(frame);
            });
            break;
    }
}

void libmediafy::VideoOutput::on_frame_available() {
    gl_executor.queue_execute([&]() {
        get_env()->CallVoidMethod(renderer_java, update_image);
        render_texture(nullptr);
    });
}

void libmediafy::VideoOutput::initEGL() {
    if (!egl_initialized) {
        display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
        if (display == EGL_NO_DISPLAY) {
            printf("EGL_NO_DISPLAY!");
            abort();
        }

        if (display == EGL_NO_DISPLAY) abort();
        EGLint major = 0, minor = 0;
        if (!eglInitialize(display, &major, &minor)) abort();

        EGLint config_attribs[] = {
                EGL_RED_SIZE, 8,
                EGL_GREEN_SIZE, 8,
                EGL_BLUE_SIZE, 8,
                EGL_ALPHA_SIZE, 8,
                EGL_RENDERABLE_TYPE, EGL_OPENGL_ES3_BIT,
                EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
                EGL_NONE,
        };
        EGLint num_configs = 0;
        if (!eglChooseConfig(display, config_attribs, &config, 1, &num_configs)) abort();

        eglOutputSurface = eglCreateWindowSurface(display, config, output_surface, nullptr);
        if (eglOutputSurface == EGL_NO_SURFACE) {
            printf("No surface!");
            abort();
        }

        EGLint context_attribs[] {
                EGL_CONTEXT_CLIENT_VERSION, 3,
                EGL_NONE,
        };
        context = eglCreateContext(display, config, nullptr, context_attribs);
        if (context == EGL_NO_CONTEXT) {
            printf("EGL No Context!");
            abort();
        }
        if (!eglMakeCurrent(display, eglOutputSurface, eglOutputSurface, context)) {
            printf("Couldn't make EGL current!");
            abort();
        }

        auto create_program = [](const char* v_shader_source, const char* f_shader_source) {
            auto check_compilation_status = [](GLuint shader) {
                int success = 1;
                glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
                if (success) {
                    printf("Compiled successfully!");
                } else {
                    char infoLog[512];
                    glGetShaderInfoLog(shader, 512, nullptr, infoLog);
                    printf("Shader not compiled: %s", infoLog);
                }
            };

            GLuint program = glCreateProgram();
            GLuint v_shader = glCreateShader(GL_VERTEX_SHADER);
            glShaderSource(v_shader, 1, &v_shader_source, nullptr);
            glCompileShader(v_shader);
            check_compilation_status(v_shader);
            glAttachShader(program, v_shader);

            GLuint f_shader = glCreateShader(GL_FRAGMENT_SHADER);
            glShaderSource(f_shader, 1, &f_shader_source, nullptr);
            glCompileShader(f_shader);
            check_compilation_status(f_shader);
            glAttachShader(program, f_shader);

            glLinkProgram(program);
            int success = 1;
            glGetProgramiv(program, GL_LINK_STATUS, &success);
            if (success) {
                printf("Linked program!");
            }

            glDeleteShader(v_shader);
            glDeleteShader(f_shader);

            return program;
        };

        const char* v_shader_source = "#version 300 es\n"
                                      "layout(location = 0) in vec4 pos;\n"
                                      "layout(location = 1) in vec2 textureCoord;\n"
                                      "out vec2 tex_coord;\n"
                                      "void main() {\n"
                                      " tex_coord = textureCoord;\n"
                                      " gl_Position = pos;\n"
                                      "}";

        const char* f_shader_source = "#version 300 es\n"
                                      "#extension GL_OES_EGL_image_external_essl3 : require\n"
                                      "precision mediump float;\n"
                                      "in vec2 tex_coord;\n"
                                      "uniform samplerExternalOES OESSampler;\n"
                                      "out vec4 color;\n"
                                      "void main()\n"
                                      "{\n"
                                      " color = texture(OESSampler, tex_coord);\n"
                                      "\n"
                                      "}\n";

        const char* f2_shader_source = "#version 300 es\n"
                                      "precision mediump float;\n"
                                      "in vec2 tex_coord;\n"
                                      "uniform sampler2D ySampler;\n"
                                      "uniform sampler2D uSampler;\n"
                                      "uniform sampler2D vSampler;\n"
                                      "out vec4 color;\n"
                                      "void main()\n"
                                      "{\n"
                                       " float r, g, b, y, u, v;\n"
                                       " // Do YUV here :)\n"
                                       " y = texture(ySampler, tex_coord).r;\n"
                                       " y = 1.1643 * (y - 0.0625);\n"
                                       " u = texture(uSampler, tex_coord).r - 0.5;\n"
                                       " v = texture(vSampler, tex_coord).r - 0.5;\n"
                                       " r = y + 1.5958 * v;\n"
                                       " g = y - 0.39173 * u - 0.81290 * v;\n"
                                       " b = y + 2.017 * u; \n"
                                       " color = vec4(r, g, b, 1.0);\n"
                                       "\n"
                                      "}\n";

        mediacodec_program = create_program(v_shader_source, f_shader_source);
        yuv_program = create_program(v_shader_source, f2_shader_source);

        glGenTextures(1, &texture);
        glGenTextures(3, yuv_textures);
        get_env()->CallVoidMethod(renderer_java, set_texture_id, (jint)texture);

        if (format == AV_PIX_FMT_MEDIACODEC) glUseProgram(mediacodec_program);
        else if (format == AV_PIX_FMT_YUV420P) glUseProgram(yuv_program);
        auto error = glGetError();
        if (error != GL_NO_ERROR) {
            printf("GL ERROR: %x", error);
            return;
        }

        egl_initialized = true;
        printf("Initialized OpenGL");
    }
}

void libmediafy::VideoOutput::destroyEGL() {
    if (!egl_initialized) return;
    glDeleteProgram(mediacodec_program);
    mediacodec_program = 0;
    glDeleteProgram(yuv_program);
    yuv_program = 0;
    get_env()->CallVoidMethod(renderer_java, reset_texture_id);
    texture = 0;
    glDeleteTextures(3, yuv_textures);
    memset(yuv_textures, 0, sizeof(yuv_textures));

    eglDestroyContext(display, context);
    context = EGL_NO_CONTEXT;
    eglDestroySurface(display, eglOutputSurface);
    eglOutputSurface = EGL_NO_SURFACE;
    eglTerminate(display);
    display = EGL_NO_DISPLAY;
    egl_initialized = false;
    printf("Destroyed OpenGL");
}

void libmediafy::VideoOutput::render_texture(std::shared_ptr<AVFrame> frame) {
    if (!egl_initialized) {
        return;
    }

    if (format == AV_PIX_FMT_MEDIACODEC) {
        glUseProgram(mediacodec_program);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_EXTERNAL_OES, texture);
        glTexParameterf(GL_TEXTURE_EXTERNAL_OES, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameterf(GL_TEXTURE_EXTERNAL_OES, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameterf(GL_TEXTURE_EXTERNAL_OES, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameterf(GL_TEXTURE_EXTERNAL_OES, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

        glUniform1i(glGetUniformLocation(mediacodec_program, "OESSampler"), 0);
        GLuint error = glGetError();
        if (error != GL_NO_ERROR) printf("GL Error: %x", error);
    } else if (format == AV_PIX_FMT_YUV420P) {

        glUseProgram(yuv_program);

        if (!frame && last_frame) frame = last_frame;

        if (frame) {
            glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, yuv_textures[0]);
            glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, frame->linesize[0], frame->height, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, frame->data[0]);

            glUniform1i(glGetUniformLocation(yuv_program, "ySampler"), 0);

            glActiveTexture(GL_TEXTURE1);
            glBindTexture(GL_TEXTURE_2D, yuv_textures[1]);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, frame->linesize[1], frame->height / 2, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, frame->data[1]);
            glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            glUniform1i(glGetUniformLocation(yuv_program, "uSampler"), 1);

            glActiveTexture(GL_TEXTURE2);
            glBindTexture(GL_TEXTURE_2D, yuv_textures[2]);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, frame->linesize[2], frame->height / 2, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, frame->data[2]);
            glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            glUniform1i(glGetUniformLocation(yuv_program, "vSampler"), 2);

            if (last_frame != frame) {
                last_frame.reset();
            }
            last_frame = frame;
        }

    } else return;

    GLfloat vertices[] {
            -1.0f, 1.0,         0.0, 0.0, // TL
            -1.0f, -1.0f,       0.0, 1.0, // BL
            1.0, -1.0f,         1.0, 1.0, // BR
            1.0, 1.0,           1.0, 0.0, // TR
    };

    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(GLfloat) * 4, vertices);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(GLfloat) * 4, &vertices[2]);

    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);

    glClearColor(1.0, 1.0, 1.0, 1.0);
    glClear(GL_COLOR_BUFFER_BIT);

    GLushort indices[] {
            0, 1, 2,
            2, 3, 0,
    };

    glDrawElements(GL_TRIANGLE_STRIP, 6, GL_UNSIGNED_SHORT, indices);
    eglSwapBuffers(display, eglOutputSurface);

    int error = glGetError();
    if (error != GL_NO_ERROR) printf("GL error: 0x%x", error);
}

void libmediafy::VideoOutput::set_subtitles_view(void* sub_textview) {
    std::unique_lock<std::mutex> lock{output_surface_lock};
    get_env()->CallVoidMethod(renderer_java, set_subtitle_textview, (jobject)sub_textview);
}

void libmediafy::VideoOutput::draw_text(const std::string& text) {
    gl_executor.queue_execute([this, text]() {
        JNIEnv* env = get_env();
        jstring str = env->NewStringUTF(text.c_str());
        env->CallVoidMethod(renderer_java, render_text, str);
        env->DeleteLocalRef(str);
    });
}

void libmediafy::VideoOutput::dispose() {
    destroyEGL();
}

extern "C"
JNIEXPORT void JNICALL
Java_tech_smallwonder_libmediafy_VideoRenderer_nativeOnFrameAvailable(JNIEnv *env, jobject thiz, jlong id) {
    auto* renderer = reinterpret_cast<libmediafy::VideoOutput*>(id);
    renderer->on_frame_available();
}

#else
namespace libmediafy{
VideoOutput::VideoOutput() {}

void VideoOutput::set_output_surface(void* surface) {}

void VideoOutput::set_subtitles_view(void* sub_view) {}

void VideoOutput::render(std::shared_ptr<AVFrame> frame) {}

void VideoOutput::draw_text(const std::string& text) {}

void VideoOutput::dispose() {}

VideoOutput::~VideoOutput() {}
}
#endif
