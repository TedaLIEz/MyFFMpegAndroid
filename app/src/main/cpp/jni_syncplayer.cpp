#include <jni.h>

#include <android/native_window.h>
#include <android/native_window_jni.h>
extern "C" {
#include <libavformat/avformat.h>
#include <libavutil/imgutils.h>
#include <libswscale/swscale.h>
#include <libswresample/swresample.h>
}

#include <unistd.h>
#include "NaiveQueue.h"
//
// Created by Jian Guo on 8/27/20.
//
typedef struct _Player {
    JavaVM* java_vm;
    jobject instance;
    jobject surface;
    jobject callback;
    AVFormatContext* format_context;
    int video_stream_index;
    AVCodecContext* video_codec_context;
    ANativeWindow* native_window;
    ANativeWindow_Buffer  window_buffer;
    uint8_t* video_out_buffer;
    struct SwsContext* sws_context;
    AVFrame* rgba_frame;
    NaiveQueue<AVPacket*>* video_queue;

    int audio_stream_index;
    AVCodecContext* audio_codec_context;
    uint8_t *audio_out_buffer;
    struct SwrContext* swr_context;
    int out_channels;
    jmethodID play_audio_track_method_id;
    NaiveQueue<AVPacket*>* audio_queue;
    double audio_clock;
} Player;

/* no AV sync correction is done if below the minimum AV sync threshold */
#define AV_SYNC_THRESHOLD_MIN 0.04
/* no AV correction is done if too big error */
#define AV_NOSYNC_THRESHOLD 10.0


// 消费载体
typedef struct _Consumer {
    Player* player;
    int stream_index;
} Consumer;


// 播放器
Player *cplayer;

// 线程相关
pthread_t produce_id, video_consume_id, audio_consume_id;


// 状态码
#define SUCCESS_CODE 1
#define FAIL_CODE -1


void player_init(Player **player, JNIEnv *env, jobject instance, jobject surface) {
    *player = (Player*) malloc(sizeof(Player));
    JavaVM* java_vm;
    env->GetJavaVM(&java_vm);
    (*player)->java_vm = java_vm;
    (*player)->instance = env->NewGlobalRef(instance);
    (*player)->surface = env->NewGlobalRef(surface);
}

int format_init(Player *player, const char* path) {
    int result;
    av_register_all();
    player->format_context = avformat_alloc_context();
    result = avformat_open_input(&(player->format_context), path, nullptr, nullptr);
    if (result < 0) {
        LOGE("SyncPlayer Error: Can not open video file");
        return result;
    }
    result = avformat_find_stream_info(player->format_context, nullptr);
    if (result < 0) {
        LOGE("SyncPlayer Error : Can not find video file stream info");
        return result;
    }
    return SUCCESS_CODE;
}

int find_stream_index(Player* player, AVMediaType type) {
    auto format_context = player->format_context;
    for (int i = 0; i < format_context->nb_streams; i++) {
        if (format_context->streams[i]->codecpar->codec_type == type) {
            return i;
        }
    }
    return -1;
}


int codec_init(Player* player, AVMediaType type) {
    int result;
    auto format_context = player->format_context;
    auto index = find_stream_index(player, type);
    if (index < 0) {
        LOGE("SyncPlayer Error: Can not find stream");
        return FAIL_CODE;
    }
    auto codec_context = avcodec_alloc_context3(nullptr);
    avcodec_parameters_to_context(codec_context, format_context->streams[index]->codecpar);
    auto codec = avcodec_find_decoder(codec_context->codec_id);
    result = avcodec_open2(codec_context, codec, nullptr);
    if (result < 0) {
        LOGE("SyncPlayer Error : Can not open codec");
        return FAIL_CODE;
    }
    if (type == AVMEDIA_TYPE_AUDIO) {
        LOGI("SyncPlayer: audio stream index: %d", index);
        player->audio_stream_index = index;
        player->audio_codec_context = codec_context;
    } else if (type == AVMEDIA_TYPE_VIDEO) {
        LOGI("SyncPlayer: video stream index: %d", index);
        player->video_stream_index = index;
        player->video_codec_context = codec_context;
    }
    return SUCCESS_CODE;
}

int video_prepare(Player* player, JNIEnv* env) {
    auto codec_context = player->video_codec_context;
    auto videoWidth = codec_context->width;
    auto videoHeight = codec_context->height;
    player->native_window = ANativeWindow_fromSurface(env, player->surface);
    if (!player->native_window) {
        LOGE("SyncPlayer Error: Can not create native window");
        return FAIL_CODE;
    }
    int result = ANativeWindow_setBuffersGeometry(player->native_window, videoWidth, videoHeight, WINDOW_FORMAT_RGBA_8888);
    if (result < 0) {
        LOGE("SyncPlayer Error: Can not set native window buffer");
        ANativeWindow_release(player->native_window);
        return FAIL_CODE;
    }

    player->rgba_frame = av_frame_alloc();
    int buffer_size = av_image_get_buffer_size(AV_PIX_FMT_RGBA, videoWidth, videoHeight, 1);
    player->video_out_buffer = (uint8_t*) av_malloc(buffer_size * sizeof(uint8_t));
    av_image_fill_arrays(player->rgba_frame->data, player->rgba_frame->linesize,
            player->video_out_buffer,
            AV_PIX_FMT_RGBA, videoWidth, videoHeight, 1);
    player->sws_context = sws_getContext(videoWidth, videoHeight, codec_context->pix_fmt,
            videoWidth, videoHeight, AV_PIX_FMT_RGBA,
            SWS_BICUBIC, nullptr, nullptr, nullptr);
    return SUCCESS_CODE;
}

int audio_prepare(Player* player, JNIEnv* env) {
    auto codec_context = player->audio_codec_context;
    player->swr_context = swr_alloc();
    player->audio_out_buffer = (uint8_t*)av_malloc(44100 * 2);
    auto out_channel_layout = AV_CH_LAYOUT_STEREO;
    auto out_format = AV_SAMPLE_FMT_S16;
    int out_sample_rate = player->audio_codec_context->sample_rate;
    swr_alloc_set_opts(player->swr_context,
            out_channel_layout, out_format, out_sample_rate,
            codec_context->channel_layout, codec_context->sample_fmt, codec_context->sample_rate, 0, nullptr);
    swr_init(player->swr_context);
    player->out_channels = av_get_channel_layout_nb_channels(AV_CH_LAYOUT_STEREO);
    jclass player_class = env->GetObjectClass(player->instance);
    jmethodID create_audio_track_method_id = env->GetMethodID(player_class, "createAudioTrack", "(II)V");
    env->CallVoidMethod(player->instance, create_audio_track_method_id, 44100, player->out_channels);
    player->play_audio_track_method_id = env->GetMethodID(player_class, "playAudioTrack", "([BI)V");
    return SUCCESS_CODE;
}

void video_play(Player* player, AVFrame* frame, JNIEnv* env) {
    int video_height = player->video_codec_context->height;
    int result = sws_scale(player->sws_context, (const uint8_t* const*)frame->data, frame->linesize, 0,
            video_height, player->rgba_frame->data, player->rgba_frame->linesize);
    result = ANativeWindow_lock(player->native_window, &(player->window_buffer), nullptr);
    if (result < 0) {
        LOGE("SyncPlayer Error: Can not lock native window");
    } else {
        auto bits = (uint8_t*) player->window_buffer.bits;
        for (int i = 0; i < video_height; i++) {
            memcpy(bits + i * player->window_buffer.stride * 4,
                    player->video_out_buffer + i * player->rgba_frame->linesize[0],
                    player->rgba_frame->linesize[0]);
        }
        ANativeWindow_unlockAndPost(player->native_window);
    }
}

void audio_play(Player* player, AVFrame* frame, JNIEnv* env) {
    swr_convert(player->swr_context, &(player->audio_out_buffer), 44100 * 2,
            (const uint8_t **) frame->data, frame->nb_samples);
    auto size = av_samples_get_buffer_size(nullptr, player->out_channels,
            frame->nb_samples, AV_SAMPLE_FMT_S16, 1);
    auto audio_sample_array = env->NewByteArray(size);
    env->SetByteArrayRegion(audio_sample_array, 0, size, (const jbyte *) player->audio_out_buffer);
    env->CallVoidMethod(player->instance, player->play_audio_track_method_id, audio_sample_array, size);
    env->DeleteLocalRef(audio_sample_array);
}


void player_release(Player* player) {
    avformat_close_input(&(player->format_context));
    av_free(player->video_out_buffer);
    av_free(player->audio_out_buffer);
    avcodec_close(player->video_codec_context);
    ANativeWindow_release(player->native_window);
    sws_freeContext(player->sws_context);
    av_frame_free(&(player->rgba_frame));
    avcodec_close(player->audio_codec_context);
    swr_free(&(player->swr_context));
    delete player->video_queue;
    delete player->audio_queue;
    player->instance = nullptr;
    JNIEnv *env;
    int result = player->java_vm->AttachCurrentThread(&env, nullptr);
    if (result != JNI_OK) {
        return;
    }
    env->DeleteGlobalRef(player->instance);
    env->DeleteGlobalRef(player->surface);
    player->java_vm->DetachCurrentThread();
}


void* produceFrame(void* arg) {
    auto player = (Player*)arg;
    auto packet = av_packet_alloc();
    for (;;) {
        if (av_read_frame(player->format_context, packet) < 0) {
            LOGE("SyncPlayer: No more frame, break produce loop");
            break;
        }
        if (packet->stream_index == player->video_stream_index) {
            player->video_queue->offer(packet);
        } else if (packet->stream_index == player->audio_stream_index) {
            player->audio_queue->offer(packet);
        }
        LOGD("SyncPlayer: alloc packet");
        packet = av_packet_alloc();
    }
    for (;;) {
        if (player->video_queue->isEmpty() && player->audio_queue->isEmpty()) {
            break;
        }
        LOGI("SyncPlayer, producer keep waiting");
        sleep(1);
    }
    player_release(player);
    return nullptr;
}



void* consumeFrame(void* arg) {
    auto consumer = (Consumer*) arg;
    auto player = consumer->player;
    auto index = consumer->stream_index;
    JNIEnv* env;
    auto result = player->java_vm->AttachCurrentThread(&env, nullptr);
    if (result != JNI_OK) {
        LOGE("SyncPlayer Error: Can not get current thread env");
        pthread_exit(nullptr);
        return nullptr;
    }
    AVCodecContext* codec_context;
    AVStream* stream;
    NaiveQueue<AVPacket*>* queue = nullptr;
    if (index == player->video_stream_index) {
        codec_context = player->video_codec_context;
        stream = player->format_context->streams[player->video_stream_index];
        queue = player->video_queue;
        video_prepare(player, env);
    } else if (index == player->audio_stream_index) {
        codec_context = player->audio_codec_context;
        stream = player->format_context->streams[player->audio_stream_index];
        queue = player->audio_queue;
        audio_prepare(player, env);
    } else {
        LOGE("SyncPlayer: consumeFrame Unknown Index: %d", index);
        pthread_exit(nullptr);
    }
    auto frame = av_frame_alloc();
    for (;;) {
        auto packet = queue->poll();
        if (!packet) {
            LOGE("SyncPlayer: No packet found in queue, quit consumer");
            break;
        }
        result = avcodec_send_packet(codec_context, packet);
        if (result < 0 && result != AVERROR(EAGAIN) && result != AVERROR_EOF) {
            LOGE("SyncPlayer Error: %d codec avcodec_send_packet fail", index);
            av_packet_free(&packet);
            continue;
        }
        result = avcodec_receive_frame(codec_context, frame);
        if (result < 0 && result != AVERROR_EOF) {
            LOGE("SyncPlayer Error: %d codec avcodec_receive_frame fail", index);
            av_packet_free(&packet);
            continue;
        }
        if (index == player->video_stream_index) {
            auto audio_clock = player->audio_clock;
            double timestamp;
            if (packet->pts == AV_NOPTS_VALUE) {
                timestamp = 0;
            } else {
                timestamp = frame->best_effort_timestamp * av_q2d(stream->time_base);
            }
            double frame_rate = av_q2d(stream->avg_frame_rate);
            frame_rate += frame->repeat_pict * (frame_rate * 0.5);
            if (timestamp == 0.0) {
                usleep(frame_rate * 1000);
            } else {
                if (fabs(timestamp - audio_clock) > AV_SYNC_THRESHOLD_MIN
                    && fabs(timestamp - audio_clock) < AV_NOSYNC_THRESHOLD) {
                    if (timestamp > audio_clock) {
                        usleep((unsigned long)((timestamp - audio_clock)*1000000));
                    }
                }
            }
            video_play(player, frame, env);
        } else if (index == player->audio_stream_index) {
            player->audio_clock = packet->pts * av_q2d(stream->time_base);
            LOGD("SyncPlayer: Playing audio loop");
            audio_play(player, frame, env);
        } else {
            LOGE("SyncPlayer Error: Playing Unknown index: %d", index);
        }
        av_packet_free(&packet);
    }
    player->java_vm->DetachCurrentThread();

    LOGE("consume is finish ------------------------------ ");
    return nullptr;

}




void thread_init(Player* player) {
    pthread_create(&produce_id, nullptr, produceFrame, player);
    auto video_consumer = (Consumer*) malloc(sizeof(Consumer));
    video_consumer->player = player;
    video_consumer->stream_index = player->video_stream_index;
    pthread_create(&video_consume_id, nullptr, consumeFrame, video_consumer);
    auto audio_consumer = (Consumer*) malloc(sizeof(Consumer));
    audio_consumer->player = player;
    audio_consumer->stream_index = player->audio_stream_index;
    pthread_create(&audio_consume_id, nullptr, consumeFrame, audio_consumer);
}

void play_start(Player* player) {
    player->video_queue = new NaiveQueue<AVPacket*>();
    player->audio_queue = new NaiveQueue<AVPacket*>();
    thread_init(player);
}

extern "C"
JNIEXPORT void JNICALL
Java_com_github_tedaliez_testffmpeg_SyncPlayer_playVideo(JNIEnv *env, jobject instance, jstring path_, jobject surface) {
    const char *path = env->GetStringUTFChars(path_, 0);
    int result = 1;
    Player* player;
    player_init(&player, env, instance, surface);
    if (result > 0) {
        result = format_init(player, path);
    }
    if (result > 0) {
        result = codec_init(player, AVMEDIA_TYPE_VIDEO);
    }
    if (result > 0) {
        result = codec_init(player, AVMEDIA_TYPE_AUDIO);
    }
    if (result > 0) {
        play_start(player);
    }
    LOGE("SyncPlayer: start playing");
    env->ReleaseStringUTFChars(path_, path);
}