#ifdef ANDROID
#include "Renderer.h"
#include "Document.h"
#include "Window.h"
#include "Audio.h"
#include "GUI.h"
#include <jni.h>
#include <android/native_window_jni.h>

static Renderer *renderer = NULL;
static Window   *window = NULL;
static Document  doc;
static GUI       gui(doc);

static void callback(JNIEnv *env, jobject obj, ButtonAction i)
{
	switch (i)
	{
		case CHANGE_IMAGE:
		{
			jclass cls = env->GetObjectClass(obj);
			jmethodID mid = env->GetMethodID(cls, "changeImage", "()V");
			assert(mid != 0);
			env->CallVoidMethod(obj, mid);
			break;
		}
		default:
			if (window) window->button_action(i);
			break;
	}
}

//-------------------------------------------------------------------------------------------------------
// Exports
//-------------------------------------------------------------------------------------------------------

extern "C" {
#define F(ret, name)       JNIEXPORT ret JNICALL Java_com_hilgenberg_jigsaw_PuzzleView_ ## name (JNIEnv *env, jclass obj)
#define FF(ret, name, ...) JNIEXPORT ret JNICALL Java_com_hilgenberg_jigsaw_PuzzleView_ ## name (JNIEnv *env, jclass obj, __VA_ARGS__)
FF(void, reinit, jobject surface, jstring path)
{
	const char *s = env->GetStringUTFChars(path, NULL);
	Preferences::directory(std::string(s, env->GetStringUTFLength(path)));
	env->ReleaseStringUTFChars(path, s);

	delete renderer; renderer = NULL;
	delete window;   window   = NULL;
	if (!Preferences::load_state(doc))
	{
		bool ok = doc.load("///sample-data", 150);
		assert(ok);
	}
	ANativeWindow *jwin = ANativeWindow_fromSurface(env, surface);
	assert(jwin != NULL); if (!jwin) return;
	try { window = new Window(doc, gui); } catch (...) { return; }
	try { renderer = new Renderer(doc, *window, gui, jwin); } catch (...) { delete window; window = NULL; return; }

}

F(void, pause)
{
	Preferences::flush();
	Preferences::save_state(doc);

	audio_pause();
	delete renderer; renderer = NULL;
}

FF(jboolean, setImage, jstring path)
{
	const char *s = env->GetStringUTFChars(path, NULL);
	bool ok = doc.load(std::string(s, env->GetStringUTFLength(path)), 200);
	env->ReleaseStringUTFChars(path, s);
	if (ok) Preferences::save_state(doc);
	return ok;
}

FF(void, resize, jint w, jint h)
{
	if (!renderer) return;
	window->reshape(w, h);
}

F(jboolean, draw)
{
	if (!renderer) return false;
	renderer->draw();
	return doc.needs_redraw();
}

FF(void, touchUni, jint ds, jint id, jfloat x, jfloat y)
{
	if (!gui.handle_touch(ds, 1, &id, &x, &y))
	if (window) window->handle_touch(ds, 1, &id, &x, &y, [env,obj](ButtonAction i){ callback(env, obj, i); });
}

FF(void, touchMulti, jint ds, jintArray id_, jfloatArray x_, jfloatArray y_)
{
	if (!window) return;

	int n = env->GetArrayLength(x_);
	assert(n == env->GetArrayLength(y_));
	assert(n == env->GetArrayLength(id_));
	jfloat *x = env->GetFloatArrayElements(x_, NULL);
	jfloat *y = env->GetFloatArrayElements(y_, NULL);
	jint  *id = env->GetIntArrayElements (id_, NULL);
	if (!gui.handle_touch(ds, n, id, x, y))
	try { window->handle_touch(ds, n, id, x, y, [env,obj](ButtonAction i){ callback(env, obj, i); }); } catch (...) {}
	env->ReleaseFloatArrayElements(x_, x, JNI_ABORT);
	env->ReleaseFloatArrayElements(y_, y, JNI_ABORT);
	env->ReleaseIntArrayElements(id_, id, JNI_ABORT);
}

};

#endif
