#include <jni.h>
#include "video_config.h"

//
// Created by Jian Guo on 8/14/20.
//

extern "C"
JNIEXPORT void JNICALL
Java_com_github_tedaliez_testffmpeg_VideoFileConfig_nativeNewFD(JNIEnv *env, jobject thiz,
                                                                jint file_descriptor) {
    video_config_new(thiz, file_descriptor);

}

extern "C"
JNIEXPORT void JNICALL
Java_com_github_tedaliez_testffmpeg_VideoFileConfig_nativeNewPath(JNIEnv *env, jobject thiz,
                                                                  jstring file_path) {
    const char *filePath = env->GetStringUTFChars(file_path, nullptr);

    video_config_new(thiz, filePath);

    env->ReleaseStringUTFChars(file_path, filePath);
}

extern "C"
JNIEXPORT void JNICALL
Java_com_github_tedaliez_testffmpeg_VideoFileConfig_release(JNIEnv *env, jobject thiz) {
    video_config_free(thiz);
}


extern "C"
JNIEXPORT jstring JNICALL
Java_com_github_tedaliez_testffmpeg_VideoFileConfig_getFileFormat(JNIEnv *env, jobject instance) {
    auto *videoConfig = video_config_get(instance);
    return env->NewStringUTF(videoConfig->avFormatContext->iformat->long_name);
}


extern "C"
JNIEXPORT jstring JNICALL
Java_com_github_tedaliez_testffmpeg_VideoFileConfig_getCodecName(JNIEnv *env, jobject instance) {
    auto *videoConfig = video_config_get(instance);
    return env->NewStringUTF(videoConfig->avVideoCodec->long_name);
}

extern "C"
JNIEXPORT jint JNICALL
Java_com_github_tedaliez_testffmpeg_VideoFileConfig_getWidth(JNIEnv *, jobject instance) {
    auto *videoConfig = video_config_get(instance);
    return videoConfig->parameters->width;
}

extern "C"
JNIEXPORT jint JNICALL
Java_com_github_tedaliez_testffmpeg_VideoFileConfig_getHeight(JNIEnv *, jobject instance) {
    auto *videoConfig = video_config_get(instance);
    return videoConfig->parameters->height;
}