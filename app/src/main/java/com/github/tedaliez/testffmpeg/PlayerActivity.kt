package com.github.tedaliez.testffmpeg

import android.graphics.PixelFormat
import android.net.Uri
import androidx.appcompat.app.AppCompatActivity
import android.os.Bundle
import android.util.Log
import android.view.SurfaceHolder
import kotlinx.android.synthetic.main.activity_main.*
import kotlinx.android.synthetic.main.activity_main.surfaceView
import kotlinx.android.synthetic.main.activity_player.*

class PlayerActivity : BasePlaygroundAct() {

    companion object {
        const val TAG = "PlayerActivity"
    }

    private val player = Player()

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
    }

}