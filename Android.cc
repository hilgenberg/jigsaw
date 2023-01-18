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

static thread_local JNIEnv   *env = NULL;
static thread_local jobject   obj = NULL;
#define SAVE_CALLER do{ assert(env); assert(obj); ::env = env; ::obj = obj; }while(0)
#define CLEAR_CALLER do{ ::env = NULL; ::obj = NULL; }while(0)

static void call_java_view(const char *method)
{
	assert(env);
	assert(obj);
	jclass cls = env->GetObjectClass(obj); assert(cls != 0);
	jmethodID mid = env->GetMethodID(cls, method, "()V"); assert(mid != 0);
	env->CallVoidMethod(obj, mid);
}

void call_change_image() { call_java_view("changeImage"); }
void buy_license()       { call_java_view("buyLicense");  }
void reload_license()    { call_java_view("reloadLicense");  }

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
	if (doc.puzzle.N < 9 && !Preferences::load_state(doc))
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

F(void, setLicensed)
{
	Preferences::cached_license(true);
	Preferences::flush();
}
F(bool, findCachedLicense)
{
	return Preferences::cached_license();
}

FF(jboolean, setImage, jstring path_)
{
	SAVE_CALLER;
	const char *s = env->GetStringUTFChars(path_, NULL);
	std::string path(s, env->GetStringUTFLength(path_));
	env->ReleaseStringUTFChars(path_, s);
	
	std::string prior = doc.puzzle.im_path;
	int N = doc.puzzle.N; if (N < 9) N = 200;

	bool ok = doc.load(path, N);
	if (ok)
		Preferences::save_state(doc);
	else
		prior = path; // delete the broken new file instead

	std::string cache = Preferences::directory();

	if (!cache.empty() && has_prefix(prior, cache)) try
	{
		if (std::filesystem::remove(prior))
			LOG_DEBUG("Removed %s from cache.", prior.c_str());
		else
			LOG_ERROR("Could not delete %s from cache", prior.c_str());
	}
	catch (const std::filesystem::filesystem_error &err)
	{
		LOG_ERROR("Could not delete %s from cache: %s", prior.c_str(), err.what());
	}
	
	CLEAR_CALLER;
	return ok;
}

FF(void, resize, jint w, jint h)
{
	if (!renderer) return;
	SAVE_CALLER;
	window->reshape(w, h);
	CLEAR_CALLER;
}

F(jint, draw)
{
	if (!renderer) return false;
	SAVE_CALLER;
	renderer->draw();
	int ret = (int)doc.needs_redraw() + 2 * (int)pending_vibration;
	pending_vibration = false;
	CLEAR_CALLER;
	return ret;
}

FF(void, touchUni, jint ds, jint id, jfloat x, jfloat y)
{
	SAVE_CALLER;
	if (!gui.handle_touch(ds, 1, &id, &x, &y))
	if (window) window->handle_touch(ds, 1, &id, &x, &y);
	CLEAR_CALLER;
}

FF(void, touchMulti, jint ds, jintArray id_, jfloatArray x_, jfloatArray y_)
{
	if (!window) return;
	SAVE_CALLER;

	int n = env->GetArrayLength(x_);
	assert(n == env->GetArrayLength(y_));
	assert(n == env->GetArrayLength(id_));
	jfloat *x = env->GetFloatArrayElements(x_, NULL);
	jfloat *y = env->GetFloatArrayElements(y_, NULL);
	jint  *id = env->GetIntArrayElements (id_, NULL);
	if (!gui.handle_touch(ds, n, id, x, y))
	try { window->handle_touch(ds, n, id, x, y); } catch (...) {}
	env->ReleaseFloatArrayElements(x_, x, JNI_ABORT);
	env->ReleaseFloatArrayElements(y_, y, JNI_ABORT);
	env->ReleaseIntArrayElements(id_, id, JNI_ABORT);
	CLEAR_CALLER;
}

}; // extern "C"
#endif // ANDROID
