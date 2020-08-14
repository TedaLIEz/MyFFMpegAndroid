package com.github.tedaliez.testffmpeg

import android.app.Activity
import android.content.Intent
import android.net.Uri
import android.view.Menu
import android.view.MenuItem
import androidx.appcompat.app.AppCompatActivity

/**
 *
 * @author fredguo
 * @since 2020/08/14
 */

abstract class BasePlaygroundAct : AppCompatActivity() {



    companion object {
        private const val REQUEST_CODE_PICK_VIDEO = 0xfffe
    }

    abstract fun onVideoPicked(uri: Uri)


    override fun onActivityResult(requestCode: Int, resultCode: Int, data: Intent?) {
        super.onActivityResult(requestCode, resultCode, data)
        if (requestCode == REQUEST_CODE_PICK_VIDEO) {
            if (resultCode == Activity.RESULT_OK && data?.data != null) {
                onVideoPicked(data.data!!)
            }
        }
    }


    override fun onCreateOptionsMenu(menu: Menu): Boolean {
        menu.add("PICKVIDEO").setOnMenuItemClickListener {
            startActivityForResult(
                Intent(Intent.ACTION_GET_CONTENT)
                    .setType("video/*")
                    .putExtra(Intent.EXTRA_LOCAL_ONLY, true)
                    .addCategory(Intent.CATEGORY_OPENABLE),
                REQUEST_CODE_PICK_VIDEO
            )
            true
        }.setShowAsAction(MenuItem.SHOW_AS_ACTION_ALWAYS)
        return true
    }

}