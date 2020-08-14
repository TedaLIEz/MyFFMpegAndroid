//
// Created by Jian Guo on 8/14/20.
//

#ifndef TESTFFMPEG_VIDEO_CONFIG_H
#define TESTFFMPEG_VIDEO_CONFIG_H

#include <jni.h>

extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
}

struct VideoConfig {
    bool fullFeatured;
    int fileDescriptor;
    AVFormatContext *avFormatContext;
    AVCodecParameters *parameters;
    AVCodec *avVideoCodec;
    int videoStreamIndex;
};

// Setting the VideoFileConfig.nativePointer is delegated to Java part
void video_config_new(jobject instance, int fd);
void video_config_new(jobject instance, const char* filePath);

VideoConfig *video_config_get(jobject jVideoConfig);

void video_config_free(jobject jVideoConfig);

#endif //TESTFFMPEG_VIDEO_CONFIG_H
