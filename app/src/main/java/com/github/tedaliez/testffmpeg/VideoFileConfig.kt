package com.github.tedaliez.testffmpeg

import android.net.Uri
import android.os.ParcelFileDescriptor

/**
 *
 * @since 2020/08/13
 */
class VideoFileConfig private constructor(val fullFeatured: Boolean, val uri: Uri) {

    private constructor(fileDescriptor: Int, uri: Uri) : this(false, uri) {
        nativeNewFD(fileDescriptor)
    }

    private constructor(filePath: String, uri: Uri) : this(true, uri) {
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


    private external fun nativeNewFD(fileDescriptor: Int)

    private external fun nativeNewPath(filePath: String)


    override fun toString(): String {
        return "VideoFileConfig(fullFeatured=$fullFeatured, uri=$uri, nativePointer=$nativePointer, " +
                "width=$width, height=$height, fileFormant=$fileFormat, codecName=$codecName)"
    }


    companion object {

        fun create(filePath: String, uri: Uri) = returnIfValid(VideoFileConfig(filePath, uri))

        fun create(descriptor: ParcelFileDescriptor, uri: Uri) = returnIfValid(VideoFileConfig(descriptor.detachFd(), uri))

        private fun returnIfValid(config: VideoFileConfig) =
            if (config.nativePointer == -1L) {
                null
            } else config


    }
}