cmake_minimum_required(VERSION 3.4.1)

project(libmediafy)

set(CMAKE_CXX_STANDARD 14)



set(EXTERNAL_DIR "${CMAKE_CURRENT_SOURCE_DIR}/external_libs")

if(ANDROID)
	list(APPEND EXTERNAL_LIBS
		"${EXTERNAL_DIR}/ffmpeg/lib/android/${ANDROID_ABI}/libffmpeg.so"
		"OpenSLES"
        "mediandk"
        "log"
        "z"
        "android"
        "GLESv3"
        "EGL"
	)
	list(APPEND EXTERNAL_INCLUDES
		"${EXTERNAL_DIR}/ffmpeg/include"
	)

elseif(UNIX)
	list(APPEND EXTERNAL_LIBS
		"z"
		"EGL"
		"GLESv2"
		"avcodec"
		"avformat"
		"avutil"
		"swresample"
		"swscale"
		"avfilter"
	)

endif(ANDROID)

include_directories(
	${EXTERNAL_INCLUDES} ${CMAKE_CURRENT_SOURCE_DIR}/src/headers ${CMAKE_CURRENT_SOURCE_DIR}/include
)

set(SOURCES
        src/Decoder.cpp
        src/lib.cpp
        src/MediaPlayer.cpp
        src/Media.cpp
        src/ThreadExecutor.cpp
        src/SubtitlesManager.cpp
        src/Encoder.cpp
        src/AudioOutput.cpp
        src/Resampler.cpp
        src/ACompressor.cpp
		src/VideoOutput.cpp)

add_library(mediafy SHARED ${SOURCES})

set_target_properties(mediafy PROPERTIES CXX_VISIBILITY_PRESET hidden)

target_link_libraries(mediafy ${EXTERNAL_LIBS})

if(ANDROID)
	install(TARGETS mediafy
	LIBRARY DESTINATION ${CMAKE_CURRENT_SOURCE_DIR}/output/android/lib/${ANDROID_ABI}
	ARCHIVE DESTINATION ${CMAKE_CURRENT_SOURCE_DIR}/output/android/lib/${ANDROID_ABI})
	install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/external_libs/ffmpeg/lib/android/${ANDROID_ABI}/libffmpeg.so DESTINATION ${CMAKE_CURRENT_SOURCE_DIR}/output/android/lib/${ANDROID_ABI})
	install(FILES include/api.h DESTINATION output/android/include)
elseif(UNIX)
	set(CMAKE_INSTALL_PREFIX ${CMAKE_CURRENT_SOURCE_DIR})
	install(TARGETS mediafy
	LIBRARY DESTINATION ${CMAKE_CURRENT_SOURCE_DIR}/output/linux/lib
	ARCHIVE DESTINATION ${CMAKE_CURRENT_SOURCE_DIR}/output/linux/lib)
	install(FILES include/api.h DESTINATION output/linux/include)
endif(ANDROID)

if (UNIX)
	add_executable(test output/linux/lib/testlibmediafy.c)
	target_link_libraries(test mediafy)
endif(UNIX)
