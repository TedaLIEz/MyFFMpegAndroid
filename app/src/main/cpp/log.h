//
// Created by Jian Guo on 8/14/20.
//

#ifndef TESTFFMPEG_LOG_H
#define TESTFFMPEG_LOG_H


#include <android/log.h>

#define LOG_TAG  "MyFFMpeg"

#ifndef NDEBUG
# define LOGV(...)  __android_log_print(ANDROID_LOG_VERBOSE, LOG_TAG, __VA_ARGS__)
# define LOGD(...)  __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)
# define LOGI(...)  __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
# define LOGW(...)  __android_log_print(ANDROID_LOG_WARNING, LOG_TAG, __VA_ARGS__)
# define LOGE(...)  __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)
#else
# define LOGV(...)  (void)0
# define LOGD(...)  (void)0
# define LOGI(...)  (void)0
# define LOGW(...)  (void)0
# define LOGE(...)  (void)0
#endif


#endif //TESTFFMPEG_LOG_H
