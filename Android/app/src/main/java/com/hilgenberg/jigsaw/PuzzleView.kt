package com.hilgenberg.jigsaw

import android.app.Activity
import android.app.Application
import android.content.Intent
import android.opengl.GLSurfaceView
import android.os.Vibrator
import android.util.Log
import android.view.MotionEvent
import android.view.Surface
import androidx.core.app.ActivityCompat.startActivityForResult
import javax.microedition.khronos.egl.EGLConfig
import javax.microedition.khronos.opengles.GL10


internal class PuzzleView(val context: Application, val activity: MainActivity) : GLSurfaceView(context)
{
	init {
		setEGLConfigChooser(8, 8, 8, 0, 8, 0)
		setEGLContextClientVersion(3)
		setRenderer(object : Renderer {
			override fun onDrawFrame(gl: GL10) {
				val r = draw()
				if (r and 1 != 0) requestRender()
				if (r and 2 != 0) vibrate()
			}
			override fun onSurfaceChanged(gl: GL10, w: Int, h: Int) { resize(w, h); requestRender() }
			override fun onSurfaceCreated(gl: GL10, config: EGLConfig) {
				reinit(holder.surface, context.filesDir.path)
				requestRender()
			}
		})
		renderMode = RENDERMODE_WHEN_DIRTY
	}

	override fun onPause() {
		pause()
		super.onPause()
	}

	override fun onTouchEvent(e: MotionEvent): Boolean
	{
		when (e.actionMasked)
		{
			MotionEvent.ACTION_DOWN, MotionEvent.ACTION_POINTER_DOWN -> {
				e.actionIndex.also { touchUni(+1, e.getPointerId(it), e.getX(it), e.getY(it)) }
				requestRender()
				return true
			}
			MotionEvent.ACTION_UP, MotionEvent.ACTION_POINTER_UP -> {
				e.actionIndex.also { touchUni(-1, e.getPointerId(it), e.getX(it), e.getY(it)) }
				requestRender()
				return true
			}
			MotionEvent.ACTION_CANCEL -> {
				val id = IntArray(0)
				val x  = FloatArray(0)
				val y  = FloatArray(0)
				touchMulti(-1, id, x, y)
				requestRender()
				return true
			}
			MotionEvent.ACTION_MOVE, MotionEvent.ACTION_OUTSIDE -> {
				val n  = e.pointerCount
				val id = IntArray(n)   { e.getPointerId(it) }
				val x  = FloatArray(n) { e.getX(it) }
				val y  = FloatArray(n) { e.getY(it) }
				touchMulti(0, id, x, y)
				requestRender()
				return true
			}
		}

		return super.onTouchEvent(e)
	}

	fun changeImage()
	{
		val intent = Intent(Intent.ACTION_GET_CONTENT)
		intent.type = "image/*"
		intent.addCategory(Intent.CATEGORY_OPENABLE)
		try {
			startActivityForResult(activity, Intent.createChooser(intent, "Select Picture"), 1 /*requestCode*/, null)
		} catch (ex: Exception) {
			Log.e("JIGSAW", "change_image: $ex")
		}
	}

	fun vibrate()
	{
		val v = context.getSystemService(Activity.VIBRATOR_SERVICE) as Vibrator
		//Log.d("JIGSAW", if (v.hasVibrator()) "Yes, can Vibrate" else "No, cannot Vibrate")
		v.vibrate(50)

		// or this:
		//performHapticFeedback(HapticFeedbackConstants.CONTEXT_CLICK, HapticFeedbackConstants.FLAG_IGNORE_GLOBAL_SETTING)
	}

	private external fun reinit(surface: Surface, data_path: String)
	public  external fun setImage(path: String) : Boolean
	private external fun pause()
	private external fun draw() : Int
	private external fun resize(width: Int, height: Int)
	private external fun touchUni(ds: Int, id: Int, x: Float, y: Float)
	private external fun touchMulti(ds: Int, id: IntArray, x: FloatArray, y: FloatArray)
	companion object { init { System.loadLibrary("jigsaw") } }
}
