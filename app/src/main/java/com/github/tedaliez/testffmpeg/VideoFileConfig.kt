package com.github.tedaliez.testffmpeg

import android.graphics.Bitmap
import android.os.ParcelFileDescriptor

/**
 *
 * @since 2020/08/13
 */
class VideoFileConfig {

    private constructor(fileDescriptor: Int) {
        nativeNewFD(fileDescriptor)
    }

    private constructor(filePath: String) {
        nativeNewPath(filePath)
    }

    // The field is handled by the native code
    private val nativePointer: Long = 0

    val fileFormat: String
        external get

    val codecName: String
        external get

    val width: Int
        external get

    val height: Int
        external get

    external fun release()

    external fun fillWithPreview(bitmap: Bitmap): Boolean

    private external fun nativeNewFD(fileDescriptor: Int)

    private external fun nativeNewPath(filePath: String)

    companion object {

        fun create(filePath: String) = returnIfValid(VideoFileConfig(filePath))

        fun create(descriptor: ParcelFileDescriptor) = returnIfValid(VideoFileConfig(descriptor.detachFd()))

        private fun returnIfValid(config: VideoFileConfig) =
            if (config.nativePointer == -1L) {
                null
            } else config

        init {
            listOf("avutil", "avcodec", "avformat", "swscale", "video-config").forEach {
                System.loadLibrary(it)
            }
        }
    }
}