package com.github.tedaliez.testffmpeg

import android.Manifest.permission.READ_EXTERNAL_STORAGE
import android.annotation.TargetApi
import android.app.Activity
import android.content.ContentResolver
import android.content.ContentUris
import android.content.Context
import android.content.Intent
import android.content.pm.PackageManager
import android.graphics.PixelFormat
import android.net.Uri
import android.os.Build
import android.os.Bundle
import android.os.Environment
import android.provider.DocumentsContract
import android.provider.MediaStore
import android.util.Log
import android.view.Menu
import android.view.MenuItem
import android.view.SurfaceHolder
import android.view.View
import android.widget.TextView
import android.widget.Toast
import androidx.appcompat.app.AppCompatActivity
import androidx.core.app.ActivityCompat
import androidx.core.content.ContextCompat
import kotlinx.android.synthetic.main.activity_main.*
import java.io.FileNotFoundException

class MainActivity : BasePlaygroundAct() {

    init {
        listOf("avutil", "avcodec", "avformat", "swscale", "test_ffmpeg").forEach {
            System.loadLibrary(it)
        }
    }

    companion object {
        private const val REQUEST_STORAGE = 0xffff
        private const val REQUEST_CODE_PICK_AUDIO = 0xfffd
        private const val TAG = "MainActivity"
    }

    private var videoFileConfig: VideoFileConfig? = null
    private var surfaceHolder: SurfaceHolder? = null

    override fun onVideoPicked(uri: Uri) {
        tryGetVideoConfig(uri)
    }

    override fun onCreateOptionsMenu(menu: Menu): Boolean {
        menu.add("PICKAUDIO").setOnMenuItemClickListener {
            startActivityForResult(
                Intent(Intent.ACTION_GET_CONTENT)
                    .setType("video/*")
                    .putExtra(Intent.EXTRA_LOCAL_ONLY, true)
                    .addCategory(Intent.CATEGORY_OPENABLE),
                REQUEST_CODE_PICK_AUDIO
            )
            true
        }.setShowAsAction(MenuItem.SHOW_AS_ACTION_ALWAYS)
        return super.onCreateOptionsMenu(menu)
    }

    override fun onActivityResult(requestCode: Int, resultCode: Int, data: Intent?) {
        super.onActivityResult(requestCode, resultCode, data)
        if (requestCode == REQUEST_CODE_PICK_AUDIO) {
            if (resultCode == Activity.RESULT_OK && data?.data != null) {
                onAudioPicked(data.data!!)
            }
        }
    }

    private fun onAudioPicked(uri: Uri) {
        Log.i(TAG, "play audio with uri: $uri")
        val path = FileUtils(this).getPath(uri)
        Thread {
            mPlayer.playAudio(path)
        }.start()
    }

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContentView(R.layout.activity_main)
        fileFormat.findViewById<TextView>(android.R.id.text1).text = getString(R.string.info_file_format)
        videoCodec.findViewById<TextView>(android.R.id.text1).text = getString(R.string.info_video_codec)
        width.findViewById<TextView>(android.R.id.text1).text = getString(R.string.info_width)
        height.findViewById<TextView>(android.R.id.text1).text = getString(R.string.info_height)
        protocol.findViewById<TextView>(android.R.id.text1).text = getString(R.string.info_protocol_title)
        uri.findViewById<TextView>(android.R.id.text1).text = getString(R.string.info_uri)
        if (ContextCompat.checkSelfPermission(this, READ_EXTERNAL_STORAGE) != PackageManager.PERMISSION_GRANTED) {
            ActivityCompat.requestPermissions(
                this,
                arrayOf(READ_EXTERNAL_STORAGE),
                REQUEST_STORAGE
            )
        }
        surfaceHolder = surfaceView.holder
        surfaceHolder!!.setFormat(PixelFormat.RGBA_8888)
        btn_player_ground.setOnClickListener {
            val intent = Intent()
            intent.setClass(this, PlayerActivity::class.java)
            startActivity(intent)
        }
    }


    override fun onRequestPermissionsResult(
        requestCode: Int,
        permissions: Array<out String>,
        grantResults: IntArray
    ) {
        super.onRequestPermissionsResult(requestCode, permissions, grantResults)
        if (requestCode == REQUEST_STORAGE) {
            if (grantResults.isNotEmpty() && grantResults[0] != PackageManager.PERMISSION_GRANTED) {
                finish()
            }
        }
    }



    private fun tryGetVideoConfig(uri: Uri) {
        var videoFileConfig: VideoFileConfig? = null

        // First, try get a file:// path
        val path = FileUtils(this).getPath(uri)
        if (path != null) {
            videoFileConfig = VideoFileConfig.create(path, uri)
        }

        // Second, try get a FileDescriptor.
        if (videoFileConfig == null) {
            Log.d(TAG, "fail to create config, try to use fd")
            try {
                val descriptor = contentResolver.openFileDescriptor(uri, "r")
                if (descriptor != null) {
                    videoFileConfig = VideoFileConfig.create(descriptor, uri)
                }
            } catch (e: FileNotFoundException) {
            }
        }

        if (videoFileConfig != null) {
            setVideoConfig(videoFileConfig)
            surfaceHolder!!.addCallback(object: SurfaceHolder.Callback {
                override fun surfaceChanged(p0: SurfaceHolder, p1: Int, p2: Int, p3: Int) {
                    Log.i(TAG, "surfaceChanged, pixelFormat: $p1, width: $p2, height: $p3")
                    playVideo(path)
                }

                override fun surfaceDestroyed(p0: SurfaceHolder) {
                    Log.i(TAG, "surfaceDestroyed")
                }

                override fun surfaceCreated(p0: SurfaceHolder) {
                    Log.i(TAG, "surfaceCreated")

                }

            })

        } else {
            Toast.makeText(this, "Fail to open file!", Toast.LENGTH_SHORT).show()
        }
    }

    private fun playVideo(path: String) {
        Thread {
            mPlayer.playVideo(path, surfaceHolder!!.surface)
        }.start()

    }


    private val mPlayer = Player()

    private fun setVideoConfig(config: VideoFileConfig) {
        Log.d(TAG, "setVideoConfig: $config")
        videoFileConfig?.release()
        videoFileConfig = config


        fileFormat.setupTwoLineView(config.fileFormat)
        videoCodec.setupTwoLineView(config.codecName)
        width.setupTwoLineView(config.width.toString())
        height.setupTwoLineView(config.height.toString())
        protocol.setupTwoLineView(getString(
            if (config.fullFeatured) {
                R.string.info_protocol_file
            } else {
                R.string.info_protocol_pipe
            }))
        uri.setupTwoLineView(config.uri.toString())
    }




    private fun View.setupTwoLineView(text2: String) {
        findViewById<TextView>(android.R.id.text2).text = text2
    }


}