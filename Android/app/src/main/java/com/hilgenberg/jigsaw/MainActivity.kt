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

	override fun onActivityResult(requestCode: Int, resultCode: Int, data: Intent?)
	{
		super.onActivityResult(requestCode, resultCode, data)

		if (requestCode != 1)  { Log.d("JIGSAW", "garbage request ID!"); return; }
		if (resultCode != RESULT_OK)  { Log.d("JIGSAW", "garbage result code!"); return; }

		if (data == null) { Log.d("JIGSAW", "no data from image picker!"); return; }
		//data.dataString?.let { Log.d("JIGSAW", it) }
		data.data?.let { uri ->
			val file = File.createTempFile("img", ".tmp", filesDir)
			file.outputStream().use { contentResolver.openInputStream(uri)?.copyTo(it) }
			Log.d("JIGSAW", file.absolutePath)
			if (!view.setImage(file.absolutePath)) Log.d("JIGSAW", "Failed to load!")
		}
	}
}
