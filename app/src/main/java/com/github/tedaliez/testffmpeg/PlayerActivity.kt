package com.github.tedaliez.testffmpeg

import android.graphics.PixelFormat
import android.net.Uri
import android.os.Bundle
import android.util.Log
import android.view.SurfaceHolder
import kotlinx.android.synthetic.main.activity_main.surfaceView
import kotlinx.android.synthetic.main.activity_player.*

class PlayerActivity : BasePlaygroundAct() {

    companion object {
        const val TAG = "PlayerActivity"
    }

    private val mPlayer = SyncPlayer()

    private lateinit var surfaceHolder: SurfaceHolder


    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContentView(R.layout.activity_player)
        surfaceHolder = surfaceView.holder
        surfaceHolder.setFormat(PixelFormat.RGBA_8888)
        nativeTest.setOnClickListener {
            Thread {
                NativeTest().testThread() // this block main thread as we call pthread_join
            }.start()
        }
    }


    override fun onVideoPicked(uri: Uri) {
        Log.i(TAG, "onVideoPicked, uri: $uri")
        val path = FileUtils(this).getPath(uri)
        surfaceHolder.addCallback(object: SurfaceHolder.Callback {
            override fun surfaceChanged(p0: SurfaceHolder, p1: Int, p2: Int, p3: Int) {
                Log.i(TAG, "surfaceChanged, pixelFormat: $p1, width: $p2, height: $p3")
                mPlayer.playVideo(path, surfaceHolder.surface)
            }

            override fun surfaceDestroyed(p0: SurfaceHolder) {
                Log.i(TAG, "surfaceDestroyed")
            }

            override fun surfaceCreated(p0: SurfaceHolder) {
                Log.i(TAG, "surfaceCreated")

            }

        })

    }

}