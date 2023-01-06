#ifdef ANDROID
#include "Window.h"
#include "Audio.h"
#include "Puzzle_Tools.h"

Window::Window(Document &doc, Renderer &renderer)
: doc(doc)
, renderer(renderer)
{
}

static inline void drag_view(Camera &camera, const ScreenCoords &d)
{
	camera.move(camera.dconvert(d));
}

void Window::handle_touch(int ds, int n, int *id, float *x, float *y)
{
	if (ds < 0 && n <= 0) // this is MotionEvent.ACTION_CANCEL
	{
		drag_pointer_id = -1;
		clicked_button = -1;
		button_pointer_id = -1;
		pointer_state.clear();
		if (dragging >= 0)
		{
			if (drag_tool_drop(doc.puzzle, doc.camera, dragging)) play_click();
			dragging = -1;
			redraw();
		}
	}
	else if (ds < 0) // touches lifting
	{
		handle_touch(0, n, id, x, y); // move to final position

		if (clicked_button >= 0 && n == 1 && *id == button_pointer_id)
		{
			for (auto &b : doc.buttons.buttons)
			{
				if (clicked_button != b.index) continue;
				if (!button_hit(b, *x, *y)) break;
				button_action(b.index);
				break;
			}
		}
		clicked_button = -1;
		button_pointer_id = -1;

		for (int i = 0; i < n; ++i)
		{
			pointer_state.erase(id[i]);
			if (drag_pointer_id == id[i])
			{
				if (drag_tool_drop(doc.puzzle, doc.camera, dragging)) play_click();
				drag_pointer_id = -1;
				dragging = -1;
				redraw();
			}
		}
	}
	else if (ds > 0) // new touches
	{
		for (int i = 0; i < n; ++i)
		{
			pointer_state[id[i]].set(x[i], y[i]);
		}

		if (pointer_state.size() == 1)
		{
			for (auto &b : doc.buttons.buttons)
			{
				if (!button_hit(b, *x, *y)) continue;
				clicked_button = b.index;
				button_pointer_id = *id;
				break;
			}
		}
		else
		{
			clicked_button = -1;
			button_pointer_id = -1;
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

		if (clicked_button >= 0) return;

		if (n == 1)
		{
			const int a = id[0];
			if (drag_pointer_id == -1)
			{
				drag_pointer_id = a;
				ScreenCoords p0 = ScreenCoords(x[0], y[0]) - delta[a]; // use initial position here!
				dragging = hit_test(p0, true);
			}
			if (dragging >= 0)
			{
				assert(drag_pointer_id == a);
				ScreenCoords md = delta[drag_pointer_id], p = pointer_state[drag_pointer_id];
				drag_tool(doc.puzzle, doc.camera, dragging, drag_rel, p, drag_v, md.x, md.y);
				if (drag_v.absq() > 1e-12) start_animations();
			}
			else
			{
				drag_view(doc.camera, -delta[a]);
			}
		}
		else if (n == 2)
		{
			const int a = id[0], b = id[1];
			const ScreenCoords &A = pointer_state[a], &B = pointer_state[b];
			drag_view(doc.camera, (delta[a] + delta[b])*-0.5);
			double r = distq(A, B);
			double r0 = distq(A - delta[a], B - delta[b]);
			double f = sqrt(r0/r); // camera.zoom handles NAN and other garbage
			doc.camera.zoom(f, (A+B)*0.5f);

			if (dragging >= 0)
			{
				assert(drag_pointer_id == a || drag_pointer_id == b);
				ScreenCoords md = delta[drag_pointer_id], p = pointer_state[drag_pointer_id];
				drag_tool(doc.puzzle, doc.camera, dragging, drag_rel, p, drag_v, md.x, md.y);
				if (drag_v.absq() > 1e-12) start_animations();
			}
		}
		// else  <-- TODO: shovel mode for n > 2
	}
}

#endif