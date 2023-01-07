#ifdef ANDROID
#include "Renderer.h"
#include "Document.h"
#include "Window.h"
#include <jni.h>

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
		case SETTINGS:
		{
			jclass cls = env->GetObjectClass(obj);
			jmethodID mid = env->GetMethodID(cls, "change_image", "()V");
			assert(mid != 0);
			env->CallVoidMethod(obj, mid);
			break;
		}
		default: if (window) window->button_action(i); break;
	}
}

extern "C" {
#define F(ret, name)       JNIEXPORT ret JNICALL Java_com_hilgenberg_jigsaw_PuzzleView_ ## name (JNIEnv *env, jclass obj)
#define FF(ret, name, ...) JNIEXPORT ret JNICALL Java_com_hilgenberg_jigsaw_PuzzleView_ ## name (JNIEnv *env, jclass obj, __VA_ARGS__)

FF(void, reinit, jstring path)
{
	const char *s = env->GetStringUTFChars(path, NULL);
	Preferences::directory(std::string(s, env->GetStringUTFLength(path)));
	env->ReleaseStringUTFChars(path, s);

	delete renderer; renderer = NULL;
	delete window;   window   = NULL;
	if (!Preferences::load_state(doc))
	{
		bool ok = doc.load("/storage/self/primary/Pictures/Telegram/IMG_20230104_173017_152.jpg", 100);
		assert(ok);
	}
	try { renderer = new Renderer(doc); } catch (...) { return; }
	try { window = new Window(doc, *renderer); } catch (...) { delete renderer; renderer = NULL; }
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
	return window->animating();
}

FF(void, touch_1uni, jint ds, jint id, jfloat x, jfloat y)
{
	if (window) window->handle_touch(ds, 1, &id, &x, &y, [env,obj](ButtonAction i){ callback(env, obj, i); });
}

FF(void, touch_1multi, jint ds, jintArray id_, jfloatArray x_, jfloatArray y_)
{
	if (!window) return;

	int n = env->GetArrayLength(x_);
	assert(n == env->GetArrayLength(y_));
	assert(n == env->GetArrayLength(id_));
	jfloat *x = env->GetFloatArrayElements(x_, NULL);
	jfloat *y = env->GetFloatArrayElements(y_, NULL);
	jint  *id = env->GetIntArrayElements (id_, NULL);
	try { window->handle_touch(ds, n, id, x, y, [env,obj](ButtonAction i){ callback(env, obj, i); }); } catch (...) {}
	env->ReleaseFloatArrayElements(x_, x, JNI_ABORT);
	env->ReleaseFloatArrayElements(y_, y, JNI_ABORT);
	env->ReleaseIntArrayElements(id_, id, JNI_ABORT);
}

};


#endif
