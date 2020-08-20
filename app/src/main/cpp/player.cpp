
//
// Created by Jian Guo on 8/14/20.
//


#include <jni.h>
#include <android/native_window.h>
#include <android/native_window_jni.h>
#include "log.h"

extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libswscale/swscale.h>
#include <libavutil/imgutils.h>
#include <libswresample/swresample.h>
}

extern "C"
JNIEXPORT void JNICALL
Java_com_github_tedaliez_testffmpeg_Player_playVideo(JNIEnv *env, jobject instance, jstring path_,
                                                     jobject surface) {
    // Record result
    int result;
    // R1 Java String -> C String
    const char *path = env->GetStringUTFChars(path_, 0);
    // Register FFmpeg components
    av_register_all();
    // R2 initializes the AVFormatContext context
    AVFormatContext *format_context = avformat_alloc_context();
    // Open Video File
    result = avformat_open_input(&format_context, path, NULL, NULL);
    if (result < 0) {
        LOGE("Player Error : Can not open video file");
        return;
    }
    // Finding Stream Information of Video Files
    result = avformat_find_stream_info(format_context, NULL);
    if (result < 0) {
        LOGE("Player Error : Can not find video file stream info");
        return;
    }
    // Find Video Encoder
    int video_stream_index = -1;
    for (int i = 0; i < format_context->nb_streams; i++) {
        // Matching Video Stream
        if (format_context->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            video_stream_index = i;
        }
    }
    // No video stream found
    if (video_stream_index == -1) {
        LOGE("Player Error : Can not find video stream");
        return;
    }
    // Initialization of Video Encoder Context
    AVCodecContext *video_codec_context = avcodec_alloc_context3(NULL);
    avcodec_parameters_to_context(video_codec_context, format_context->streams[video_stream_index]->codecpar);
    // Initialization of Video Encoder
    AVCodec *video_codec = avcodec_find_decoder(video_codec_context->codec_id);
    if (video_codec == NULL) {
        LOGE("Player Error : Can not find video codec");
        return;
    }
    // R3 Opens Video Decoder
    result  = avcodec_open2(video_codec_context, video_codec, NULL);
    if (result < 0) {
        LOGE("Player Error : Can not find video stream");
        return;
    }
    // Getting the Width and Height of Video
    int videoWidth = video_codec_context->width;
    int videoHeight = video_codec_context->height;
    // R4 Initializes Native Window s for Playing Videos
    ANativeWindow *native_window = ANativeWindow_fromSurface(env, surface);
    if (native_window == NULL) {
        LOGE("Player Error : Can not create native window");
        return;
    }
    // Limit the number of pixels in the buffer by setting the width, not the physical display size of the screen.
    // If the buffer does not match the display size of the physical screen, the actual display may be stretched or compressed images.
    result = ANativeWindow_setBuffersGeometry(native_window, videoWidth, videoHeight,WINDOW_FORMAT_RGBA_8888);
    if (result < 0){
        LOGE("Player Error : Can not set native window buffer");
        ANativeWindow_release(native_window);
        return;
    }
    // Define drawing buffer
    ANativeWindow_Buffer window_buffer;
    // There are three declarative data containers
    // Data container Packet encoding data before R5 decoding
    AVPacket *packet = av_packet_alloc();
    av_init_packet(packet);
    // Frame Pixel Data of Data Container After R6 Decoding Can't Play Pixel Data Directly and Need Conversion
    AVFrame *frame = av_frame_alloc();
    // R7 converted data container where the data can be used for playback
    AVFrame *rgba_frame = av_frame_alloc();
    // Data format conversion preparation
    // Output Buffer
    int buffer_size = av_image_get_buffer_size(AV_PIX_FMT_RGBA, videoWidth, videoHeight, 1);
    // R8 Application for Buffer Memory
    uint8_t *out_buffer = (uint8_t *) av_malloc(buffer_size * sizeof(uint8_t));
    LOGI("outBuffer size: %d, videoWidth: %d, videoHeight: %d, pix_fmt: %d", buffer_size * sizeof(uint8_t), videoWidth, videoHeight, video_codec_context->pix_fmt);
    av_image_fill_arrays(rgba_frame->data, rgba_frame->linesize, out_buffer, AV_PIX_FMT_RGBA, videoWidth, videoHeight, 1);
    // R9 Data Format Conversion Context
    struct SwsContext *data_convert_context = sws_getContext(
            videoWidth, videoHeight, video_codec_context->pix_fmt,
            videoWidth, videoHeight, AV_PIX_FMT_RGBA,
            SWS_BICUBIC, NULL, NULL, NULL);
    // Start reading frames
    LOGD("Start playing video, frame_rate: %d/%d, time_base: %d/%d",
            format_context->streams[video_stream_index]->r_frame_rate.den,
            format_context->streams[video_stream_index]->r_frame_rate.num,
            format_context->streams[video_stream_index]->time_base.den,
            format_context->streams[video_stream_index]->time_base.num);
    while (av_read_frame(format_context, packet) >= 0) {
        // Matching Video Stream
        if (packet->stream_index == video_stream_index) {
            // Decode
            result = avcodec_send_packet(video_codec_context, packet);
            if (result < 0 && result != AVERROR(EAGAIN) && result != AVERROR_EOF) {
                LOGE("Player Error : codec step 1 fail");
                return;
            }
            result = avcodec_receive_frame(video_codec_context, frame);
            if (result < 0 && result != AVERROR_EOF) {
                LOGE("Player Error : codec step 2 fail");
                return;
            }
            LOGI("Frame %c (%d, size=%d) pts %d dts %d key_frame %d [codec_picture_number %d, display_picture_number %d]",
                    av_get_picture_type_char(frame->pict_type), video_codec_context->frame_number,
                    frame->pkt_size,
                    frame->pts,
                    frame->pkt_dts,
                    frame->key_frame, frame->coded_picture_number, frame->display_picture_number);
            // Data Format Conversion
            result = sws_scale(
                    data_convert_context,
                    frame->data, frame->linesize,
                    0, videoHeight,
                    rgba_frame->data, rgba_frame->linesize);
//            LOGD("frame width: %d, frame height: %d", frame->width, frame->height);
//            if (result <= 0) {
//                LOGE("Player Error : data convert fail, result: %d", result);
//                return;
//            }
            // play
            result = ANativeWindow_lock(native_window, &window_buffer, NULL);
            if (result < 0) {
                LOGE("Player Error : Can not lock native window");
            } else {
                // Draw the image onto the interface
                // Note: The pixel lengths of rgba_frame row and window_buffer row may not be the same here.
                // Need to convert well or maybe screen
                uint8_t *bits = (uint8_t *) window_buffer.bits;
                for (int h = 0; h < videoHeight; h++) {
                    memcpy(bits + h * window_buffer.stride * 4,
                           out_buffer + h * rgba_frame->linesize[0],
                           rgba_frame->linesize[0]);
                }
                ANativeWindow_unlockAndPost(native_window);
            }
        }
        // Release package references
        av_packet_unref(packet);
    }
    // Release R9
    sws_freeContext(data_convert_context);
    // Release R8
    av_free(out_buffer);
    // Release R7
    av_frame_free(&rgba_frame);
    // Release R6
    av_frame_free(&frame);
    // Release R5
    av_packet_free(&packet);
    // Release R4
    ANativeWindow_release(native_window);
    // Close R3
    avcodec_close(video_codec_context);
    // Release R2
    avformat_close_input(&format_context);
    // Release R1
    env->ReleaseStringUTFChars(path_, path);


}




extern "C"
JNIEXPORT void JNICALL
Java_com_github_tedaliez_testffmpeg_Player_playAudio(JNIEnv *env, jobject instance, jstring path_) {
    int result;

    const char* path = env->GetStringUTFChars(path_, 0);
    av_register_all();

    auto format_context = avformat_alloc_context();
    avformat_open_input(&format_context, path, nullptr, nullptr);

    result = avformat_find_stream_info(format_context, nullptr);
    if (result < 0) {
        LOGE("Player Error: Can not find video file stream: %s", path);
        return;
    }

    int audio_stream_index = -1;
    for (int i = 0; i < format_context->nb_streams; i++) {
        if (format_context->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
            audio_stream_index = i;
        }
    }
    if (audio_stream_index == -1) {
        LOGE("Player Error: Can not find audio stream");
        return;
    }
    auto audio_codec_context = avcodec_alloc_context3(nullptr);
    avcodec_parameters_to_context(audio_codec_context, format_context->streams[audio_stream_index]->codecpar);

    // init codec
    auto audio_codec = avcodec_find_decoder(audio_codec_context->codec_id);
    if (!audio_codec) {
        LOGE("Player Error: Can not find audio codec");
        return;
    }

    result = avcodec_open2(audio_codec_context, audio_codec, nullptr);
    if (result < 0) {
        LOGE("Player Error: Can not open audio codec");
        return;
    }

    auto swr_context = swr_alloc();
    auto out_buffer = (uint8_t *) av_malloc(44100 * 2);


    // expected sample output
    uint64_t out_channel_layout = AV_CH_LAYOUT_STEREO;
    auto out_format = AV_SAMPLE_FMT_S16;

    auto out_sample_rate = audio_codec_context->sample_rate;

    // expected sample out para end

    swr_alloc_set_opts(swr_context,
        out_channel_layout, out_format, out_sample_rate,
        audio_codec_context->channel_layout, audio_codec_context->sample_fmt, audio_codec_context->sample_rate,
0, nullptr);


    swr_init(swr_context);

    auto out_channels = av_get_channel_layout_nb_channels(AV_CH_LAYOUT_STEREO);

    auto player_class = env->GetObjectClass(instance);
    auto create_audio_track_method_id = env->GetMethodID(player_class, "createAudioTrack", "(II)V");
    env->CallVoidMethod(instance, create_audio_track_method_id, 44100, out_channels);


    auto play_audio_track_method_id = env->GetMethodID(player_class, "playAudioTrack", "([BI)V");


    auto packet = av_packet_alloc();

    auto frame = av_frame_alloc();

    while (av_read_frame(format_context, packet) >= 0) {
        if (packet->stream_index == audio_stream_index) {
            result = avcodec_send_packet(audio_codec_context, packet);
            if (result < 0 && result != AVERROR(EAGAIN) && result != AVERROR_EOF) {
                LOGE("Player Error : codec step 1 fail");
                return;
            }
            result = avcodec_receive_frame(audio_codec_context, frame);
            if (result < 0 && result != AVERROR_EOF) {
                LOGE("Player Error : codec step 2 fail");
                return;
            }
            // resample
            swr_convert(swr_context, &out_buffer, 44100 * 2, (const uint8_t **) frame->data, frame->nb_samples);


            // play audio via JNI
            int size = av_samples_get_buffer_size(nullptr, out_channels, frame->nb_samples, AV_SAMPLE_FMT_S16, 1);
            jbyteArray  audio_sample_array = env->NewByteArray(size);
            env->SetByteArrayRegion(audio_sample_array, 0, size, (const jbyte *) out_buffer);
            env->CallVoidMethod(instance, play_audio_track_method_id, audio_sample_array, size);
            env->DeleteLocalRef(audio_sample_array);
        }
        av_packet_unref(packet);
    }

    // release AudioTrack
    jmethodID release_audio_track_method_id = env->GetMethodID(player_class, "releaseAudioTrack", "()V");
    env->CallVoidMethod(instance, release_audio_track_method_id);

    av_frame_free(&frame);
    av_packet_free(&packet);
    swr_free(&swr_context);
    avcodec_close(audio_codec_context);
    avformat_close_input(&format_context);
    avformat_free_context(format_context);
    env->ReleaseStringUTFChars(path_, path);
}