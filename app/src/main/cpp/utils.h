//
// Created by Jian Guo on 8/14/20.
//

#ifndef TESTFFMPEG_UTILS_H
#define TESTFFMPEG_UTILS_H

#include <jni.h>

int utils_fields_init(JavaVM *vm);

void utils_fields_free(JavaVM *vm);

JNIEnv *utils_get_env();

// The approach was taken from here:
// https://code.videolan.org/videolan/vlc-android/blob/master/libvlc/jni/utils.h
// https://code.videolan.org/videolan/vlc-android/blob/master/libvlc/jni/libvlcjni.c

struct fields {
    struct {
        jclass clazz;
        jfieldID nativePointer;
    } VideoFileConfig;
};

extern struct fields fields;


#endif //TESTFFMPEG_UTILS_H
