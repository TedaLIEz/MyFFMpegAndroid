cmake_minimum_required(VERSION 3.4.1)

set(ffmpeg_dir ${CMAKE_SOURCE_DIR}/../ffmpeg-android-maker/output)
include_directories(${ffmpeg_dir}/include/${ANDROID_ABI})
set(ffmpeg_libs ${ffmpeg_dir}/lib/${ANDROID_ABI})



add_library(avutil SHARED IMPORTED)
set_target_properties(avutil PROPERTIES IMPORTED_LOCATION ${ffmpeg_libs}/libavutil.so)

add_library(avformat SHARED IMPORTED)
set_target_properties(avformat PROPERTIES IMPORTED_LOCATION ${ffmpeg_libs}/libavformat.so)

add_library(avfilter SHARED IMPORTED)
set_target_properties(avfilter PROPERTIES IMPORTED_LOCATION ${ffmpeg_libs}/libavfilter.so)

add_library(avcodec SHARED IMPORTED)
set_target_properties(avcodec PROPERTIES IMPORTED_LOCATION ${ffmpeg_libs}/libavcodec.so)

add_library(swscale SHARED IMPORTED)
set_target_properties(swscale PROPERTIES IMPORTED_LOCATION ${ffmpeg_libs}/libswscale.so)

add_library(swresample SHARED IMPORTED)
set_target_properties(swresample PROPERTIES IMPORTED_LOCATION ${ffmpeg_libs}/libswresample.so)

find_library(log-lib log)
find_library(jnigraphics-lib jnigraphics)

add_library(test_ffmpeg
        SHARED
        src/main/cpp/video_config.cpp
        src/main/cpp/video_config_jni.cpp
        src/main/cpp/utils.cpp
        src/main/cpp/main.cpp
        src/main/cpp/queue.cpp
        src/main/cpp/queueTest.cpp
        src/main/cpp/player.cpp)

target_link_libraries(
        test_ffmpeg
        ${log-lib}
        ${jnigraphics-lib}
        android
        avformat
        avcodec
        swscale
        avutil
        swresample
        avfilter)