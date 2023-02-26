package com.hilgenberg.jigsaw
import android.app.Activity
import android.content.Intent
import android.os.Bundle
import android.util.Log
import java.io.File

class MainActivity : Activity()
{
	private lateinit var view: PuzzleView

	override fun onCreate(b: Bundle?) {
		super.onCreate(b)
		//StrictMode.setVmPolicy(VmPolicy.Builder(StrictMode.getVmPolicy()).detectLeakedClosableObjects().build())
		view = PuzzleView(application, this)
		setContentView(view)
		view.requestRender()
	}

	override fun onResume() {
		super.onResume()
		view.onResume()
		view.requestRender()
	}

	override fun onPause() {
		view.onPause()
		super.onPause()
	}

	override fun onBackPressed()
	{
		if (view.back())
		{
			view.requestRender()
		}
		else
		{
			@Suppress("OverridingDeprecatedMember", "DEPRECATION")
			super.onBackPressed()
		}
	}

	override fun onActivityResult(requestCode: Int, resultCode: Int, data: Intent?)
	{
		// only called from the image chooser (requestCode == 1)
		super.onActivityResult(requestCode, resultCode, data)
		if (requestCode != 1)  { Log.e("JIGSAW", "garbage request ID!"); return; }
		if (resultCode != RESULT_OK)  { Log.d("JIGSAW", "result code: $resultCode"); return; }
		if (data == null) { Log.e("JIGSAW", "no data from image picker!"); return; }
		data.data?.let { uri ->
			val file = File.createTempFile("img", ".tmp", filesDir)
			file.outputStream().use { of -> contentResolver.openInputStream(uri)?.use { it.copyTo(of) }}
			if (!view.setImage(file.absolutePath)) Log.e("JIGSAW", "Failed to load image!")
		}
	}
}
