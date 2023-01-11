#ifdef ANDROID
#include "Renderer.h"
#include "Document.h"
#include "Window.h"
#include "Audio.h"
#include "GUI.h"
#include <jni.h>
#include <android/native_window_jni.h>

//-------------------------------------------------------------------------------------------------------
// Exports
//-------------------------------------------------------------------------------------------------------

static Renderer *renderer = NULL;
static Window   *window = NULL;
static Document  doc;

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

extern "C" {
#define F(ret, name)       JNIEXPORT ret JNICALL Java_com_hilgenberg_jigsaw_PuzzleView_ ## name (JNIEnv *env, jclass obj)
#define FF(ret, name, ...) JNIEXPORT ret JNICALL Java_com_hilgenberg_jigsaw_PuzzleView_ ## name (JNIEnv *env, jclass obj, __VA_ARGS__)
FF(void, reinit, jobject surface, jstring path)
{
	const char *s = env->GetStringUTFChars(path, NULL);
	Preferences::directory(std::string(s, env->GetStringUTFLength(path)));
	env->ReleaseStringUTFChars(path, s);

	delete GUI::gui; assert(GUI::gui == NULL);
	delete renderer; renderer = NULL;
	delete window;   window   = NULL;
	if (!Preferences::load_state(doc))
	{
		bool ok = doc.load("///sample-data", 150);
		assert(ok);
	}
	try { renderer = new Renderer(doc); } catch (...) { return; }
	try { window = new Window(doc, *renderer); } catch (...) { delete renderer; renderer = NULL; return; }

	ANativeWindow *jwin = ANativeWindow_fromSurface(env, surface);
	assert(jwin != NULL);
	if (jwin) new GUI(jwin, *window); // d'tor will call ANativeWindow_release
	assert(GUI::gui != NULL);
}

F(void, pause)
{
	audio_pause();
}

FF(jboolean, setImage, jstring path)
{
	const char *s = env->GetStringUTFChars(path, NULL);
	bool ok = doc.load(std::string(s, env->GetStringUTFLength(path)), 200);
	env->ReleaseStringUTFChars(path, s);
	if (ok) Preferences::save_state(doc);
	return ok;
}

F(void, save)
{
	Preferences::flush();
	Preferences::save_state(doc);
}

FF(void, resize, jint w, jint h)
{
	if (!renderer) return;
	window->reshape(w, h);
}

F(jboolean, draw)
{
	if (!renderer) return false;
	window->animate();
	renderer->draw();
	if (GUI::gui) GUI::gui->draw();
	return window->animating() || (GUI::gui && GUI::gui->needs_redraw());
}

FF(void, touchUni, jint ds, jint id, jfloat x, jfloat y)
{
	if (!GUI::gui || !GUI::gui->handle_touch(ds, 1, &id, &x, &y)) 
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
	if (!GUI::gui || !GUI::gui->handle_touch(ds, n, id, x, y)) 
	try { window->handle_touch(ds, n, id, x, y, [env,obj](ButtonAction i){ callback(env, obj, i); }); } catch (...) {}
	env->ReleaseFloatArrayElements(x_, x, JNI_ABORT);
	env->ReleaseFloatArrayElements(y_, y, JNI_ABORT);
	env->ReleaseIntArrayElements(id_, id, JNI_ABORT);
}

};

#endif
