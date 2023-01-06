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
		pointer_state.clear();
		clicked_button = -1;
		drop();
	}
	else if (ds < 0) // touches lifting
	{
		handle_touch(0, n, id, x, y); // move to final position
		for (int i = 0; i < n; ++i) pointer_state.erase(id[i]);

		drop(); // even if something weird is going on and this is a different pointer

		if (clicked_button >= 0 && n == 1 && pointer_state.empty())
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
	}
	else if (ds > 0) // new touches
	{
		clicked_button = -1;
		drop();

		if (!va && n == 1 && pointer_state.empty())
		{
			for (auto &b : doc.buttons.buttons)
			{
				if (!button_hit(b, *x, *y)) continue;
				clicked_button = b.index;
				break;
			}

			if (clicked_button < 0) switch (doc.tool)
			{
				case Tool::SHOVEL:
					break;
				case Tool::HIDE:
				{
					int d = hit_test(ScreenCoords(*x, *y), false);
					if (d < 0) break;
					hide_tool(doc.puzzle, d, true);
					start_animations();
					break;
				}
				case Tool::MAGNET:
					dragging = hit_test(ScreenCoords(*x, *y), true);
					if (dragging < 0) break;
					magnetized.insert(dragging);
					if (doc.puzzle.g[dragging] >= 0)
					{
						auto &G = doc.puzzle.groups[doc.puzzle.g[dragging]];
						magnetized.insert(G.begin(), G.end());
					}
					redraw();
					break;
				case Tool::NONE:
					dragging = hit_test(ScreenCoords(*x, *y), true);
					if (dragging < 0) break;
					redraw();
					break;
			}
		}

		for (int i = 0; i < n; ++i) pointer_state[id[i]].set(x[i], y[i]);
	}
	else if (n > 0) // movement
	{
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

		if (pointer_state.size() != 1)
		{
			clicked_button = -1;
			drop();
		}

		if (va) return;

		redraw();

		if (pointer_state.size() == 1)
		{
			if (clicked_button >= 0 || doc.tool == Tool::HIDE) return;

			assert(n == 1);
			assert(pointer_state.count(*id));
			assert(delta.count(*id));

			ScreenCoords md = delta[*id], p = pointer_state[*id];

			if (dragging >= 0)
			{
				drag_tool(doc.puzzle, doc.camera, dragging, magnetized, drag_rel, p, drag_v, md.x, md.y);
				if (drag_v.absq() > 1e-12) start_animations();
				return;
			}

			drag_v.clear();

			if (doc.tool == Tool::SHOVEL)
			{
				shovel_tool(doc.puzzle, doc.camera, p, md);
				return;
			}
			
			drag_view(doc.camera, -md);
		}
		else if (pointer_state.size() == 2)
		{
			auto it = pointer_state.begin();
			const int a = it->first; const ScreenCoords &A = it->second;
			++it;
			const int b = it->first; const ScreenCoords &B = it->second;

			drag_view(doc.camera, (delta[a] + delta[b])*-0.5);
			double r = distq(A, B);
			double r0 = distq(A - delta[a], B - delta[b]);
			double f = sqrt(r0/r); // camera.zoom handles NAN and other garbage
			doc.camera.zoom(f, (A+B)*0.5f);
		}
		#ifdef DEBUG
		else if (!va)
		{
			va.reset(new VictoryAnimation(doc.puzzle, doc.camera));
			start_animations();
		}
		#endif
	}
}

#endif