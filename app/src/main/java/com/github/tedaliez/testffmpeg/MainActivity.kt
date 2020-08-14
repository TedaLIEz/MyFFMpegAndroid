package com.github.tedaliez.testffmpeg

import android.Manifest.permission.READ_EXTERNAL_STORAGE
import android.app.Activity
import android.content.Intent
import android.content.pm.PackageManager
import android.net.Uri
import android.os.Bundle
import android.util.Log
import android.view.Menu
import android.view.MenuItem
import android.view.View
import android.widget.TextView
import android.widget.Toast
import androidx.appcompat.app.AppCompatActivity
import androidx.core.app.ActivityCompat
import androidx.core.content.ContextCompat
import kotlinx.android.synthetic.main.activity_main.*
import java.io.FileNotFoundException

class MainActivity : AppCompatActivity() {

    companion object {
        private const val REQUEST_STORAGE = 0xffff
        private const val REQUEST_CODE_PICK_VIDEO = 0xfffe
        private const val TAG = "MainActivity"
    }

    private var videoFileConfig: VideoFileConfig? = null


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

    override fun onActivityResult(requestCode: Int, resultCode: Int, data: Intent?) {
        super.onActivityResult(requestCode, resultCode, data)
        if (requestCode == REQUEST_CODE_PICK_VIDEO) {
            if (resultCode == Activity.RESULT_OK && data?.data != null) {
                tryGetVideoConfig(data.data!!)
            }
        }
    }


    private fun tryGetVideoConfig(uri: Uri) {
        var videoFileConfig: VideoFileConfig? = null

        // First, try get a file:// path
        val path = PathUtil.getPath(this, uri)
        if (path != null) {
            videoFileConfig = VideoFileConfig.create(path, uri)
        }

        // Second, try get a FileDescriptor.
        if (videoFileConfig == null) {
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
        } else {
            Toast.makeText(this, "Fail to open file!", Toast.LENGTH_SHORT).show()
        }
    }

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


    override fun onCreateOptionsMenu(menu: Menu): Boolean {
        menu.add("PICKVIDEO").setOnMenuItemClickListener {
            startActivityForResult(
                Intent(Intent.ACTION_GET_CONTENT)
                .setType("video/*")
                .putExtra(Intent.EXTRA_LOCAL_ONLY, true)
                .addCategory(Intent.CATEGORY_OPENABLE),
                REQUEST_CODE_PICK_VIDEO)
            true
        }.setShowAsAction(MenuItem.SHOW_AS_ACTION_ALWAYS)
        return true
    }


    private fun View.setupTwoLineView(text2: String) {
        findViewById<TextView>(android.R.id.text2).text = text2
    }


}