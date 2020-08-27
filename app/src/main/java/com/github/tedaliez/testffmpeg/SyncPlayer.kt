package com.github.tedaliez.testffmpeg

import android.media.AudioFormat
import android.media.AudioManager
import android.media.AudioTrack
import android.util.Log
import android.view.Surface
import java.lang.annotation.Native

/**
 *
 * @author fredguo
 * @since 2020/08/27
 */
class SyncPlayer {


    companion object {
        private const val TAG = "SyncPlayer"
    }
    @Native
    private var mAudioTrack : AudioTrack? = null


    external fun playVideo(path: String, surface: Surface)

    /**
     * 创建 AudioTrack
     * 由 C 反射调用
     * @param sampleRate  采样率
     * @param channels     通道数
     */
    fun createAudioTrack(sampleRate: Int, channels: Int) {
        val channelConfig: Int
        channelConfig = if (channels == 1) {
            AudioFormat.CHANNEL_OUT_MONO
        } else if (channels == 2) {
            AudioFormat.CHANNEL_OUT_STEREO
        } else {
            AudioFormat.CHANNEL_OUT_MONO
        }
        val bufferSize = AudioTrack.getMinBufferSize(
            sampleRate,
            channelConfig,
            AudioFormat.ENCODING_PCM_16BIT
        )
        mAudioTrack = AudioTrack(
            AudioManager.STREAM_MUSIC, sampleRate, channelConfig,
            AudioFormat.ENCODING_PCM_16BIT, bufferSize, AudioTrack.MODE_STREAM
        )
        mAudioTrack?.play()
    }

    /**
     * 播放 AudioTrack
     * 由 C 反射调用
     * @param data
     * @param length
     */
    fun playAudioTrack(data: ByteArray?, length: Int) {
        if (mAudioTrack != null && mAudioTrack?.getPlayState() == AudioTrack.PLAYSTATE_PLAYING) {
            mAudioTrack?.write(data!!, 0, length)
        }
    }

    /**
     * 释放 AudioTrack
     * 由 C 反射调用
     */
    fun releaseAudioTrack() {
        if (mAudioTrack != null) {
            if (mAudioTrack?.getPlayState() == AudioTrack.PLAYSTATE_PLAYING) {
                mAudioTrack?.stop()
            }
            mAudioTrack?.release()
            mAudioTrack = null
        }
    }


}