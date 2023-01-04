#ifdef ANDROID
#include "Window.h"
#include "Audio.h"
#include <jni.h>
#include <random>
#include <android/log.h>

Window::Window(Document &doc)
: context(eglGetCurrentContext())
, doc(doc)
, buttons(*this)
{
}

Window::~Window()
{
	//if (eglGetCurrentContext() == context) cleanup(...);
}

void Window::handle_touch(int ds, int n, int *id, float *x, float *y)
{
	if (ds < 0 && n <= 0) // this is MotionEvent.ACTION_CANCEL
	{
		drag_pointer_id = -1;
		pointer_state.clear();
		if (dragging >= 0)
		{
			if (doc.drop(dragging)) play_click();
			dragging = -1;
			redraw();
		}
	}
	else if (ds < 0)
	{
		handle_touch(0, n, id, x, y); // move to final position

		for (int i = 0; i < n; ++i)
		{
			pointer_state.erase(id[i]);
			if (drag_pointer_id == id[i])
			{
				if (doc.drop(dragging)) play_click();
				drag_pointer_id = -1;
				dragging = -1;
				redraw();
			}
		}
	}
	else if (ds > 0)
	{
		for (int i = 0; i < n; ++i)
		{
			pointer_state[id[i]].set(x[i], y[i]);
		}
	}
	else
	{
		if (n <= 0) return;

		redraw();

		std::map<int, ScreenCoords> delta;
		for (int i = 0; i < n; ++i)
		{
			ScreenCoords p(x[i], y[i]);
			auto it = pointer_state.find(id[i]);
			if (it == pointer_state.end())
			{
				delta[id[i]].clear(); // should not happen though
				pointer_state.insert(std::make_pair(id[i], p));
			}
			else
			{
				delta[id[i]] = p - it->second;
				it->second = p;
			}
		}
		if (!pointer_state.count(drag_pointer_id)) { drag_pointer_id = -1; dragging = -1; }

		if (n == 1)
		{
			const int a = id[0];
			if (drag_pointer_id == -1)
			{
				drag_pointer_id = a;
				ScreenCoords p0 = ScreenCoords(x[0], y[0]) - delta[a]; // use initial position here!
				dragging = doc.hit_test(p0, true, drag_rel);
			}
			if (dragging >= 0)
			{
				assert(drag_pointer_id == a);
				ScreenCoords md = delta[drag_pointer_id], p = pointer_state[drag_pointer_id];
				doc.drag(dragging, drag_rel, p, drag_v, md.x, md.y);
				if (drag_v.absq() > 1e-12) start_animations();
			}
			else
			{
				doc.drag_view(delta[a]);
			}
		}
		else if (n == 2)
		{
			const int a = id[0], b = id[1];
			const ScreenCoords &A = pointer_state[a], &B = pointer_state[b];
			doc.drag_view((delta[a] + delta[b])*0.5);
			double r = distq(A, B);
			double r0 = distq(A - delta[a], B - delta[b]);
			doc.zoom(sqrt(r/r0), (A+B)*0.5f); // method handles NAN and other garbage
			if (dragging >= 0)
			{
				assert(drag_pointer_id == a || drag_pointer_id == b);
				ScreenCoords md = delta[drag_pointer_id], p = pointer_state[drag_pointer_id];
				doc.drag(dragging, drag_rel, p, drag_v, md.x, md.y);
				if (drag_v.absq() > 1e-12) start_animations();
			}
		}
		// else  <-- TODO: shovel mode for n > 2
	}
}

//-------------------------------------------------------------------------------------------------------
// Exports
//-------------------------------------------------------------------------------------------------------

static Window *window = NULL;
static Document doc;

#define FF(ret, name, ...) JNIEXPORT ret JNICALL Java_com_hilgenberg_jigsaw_MainView_ ## name (JNIEnv *env, jclass obj, __VA_ARGS__)
#define F(ret, name) JNIEXPORT ret JNICALL Java_com_hilgenberg_jigsaw_MainView_ ## name (JNIEnv *env, jclass obj)

extern "C" {

F(void, create)
{
	delete window;
	window = NULL;
	try { window = new Window(doc); } catch (...) {}
}

FF(void, resize, jint w, jint h)
{
	if (window) window->reshape(w, h);
}

F(jboolean, draw)
{
	if (!window) return false;
	window->animate();
	window->draw();
	return window->animating();
}

FF(void, touch_uni, jint ds, jint id, jfloat x, jfloat y)
{
	if (window) window->handle_touch(ds, 1, &id, &x, &y);
}

FF(void, touch_multi, jint ds, jintArray id_, jfloatArray x_, jfloatArray y_)
{
	if (!window) return;

	int n = env->GetArrayLength(x_);
	assert(n == env->GetArrayLength(y_));
	assert(n == env->GetArrayLength(id_));
	jfloat *x = env->GetFloatArrayElements(x_, NULL);
	jfloat *y = env->GetFloatArrayElements(y_, NULL);
	jint  *id = env->GetIntArrayElements (id_, NULL);
	try { window->handle_touch(ds, n, id, x, y); } catch (...) {}
	env->ReleaseFloatArrayElements(x_, x, JNI_ABORT);
	env->ReleaseFloatArrayElements(y_, y, JNI_ABORT);
	env->ReleaseIntArrayElements(id_, id, JNI_ABORT);
}

};


#endif