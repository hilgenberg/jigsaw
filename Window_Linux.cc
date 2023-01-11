#ifdef LINUX
#include "Window.h"
#include "Document.h"
#include "Camera.h"
#include "Audio.h"
#include "Utility/StringFormatting.h"
#include "Utility/Preferences.h"
#include "Utility/GL_Util.h"
#include "Utility/Timer.h"
#include "Utility/Preferences.h"
#include "Victory.h"
#include "Puzzle_Tools.h"
#include "Buttons.h"
#include "GUI.h"
#include <SDL.h>
extern volatile bool quit;

static inline double absmax(double a, double b){ return fabs(a) > fabs(b) ? a : b; }

Window::Window(SDL_Window* window, Document &doc, Renderer &renderer)
: window(window)
, doc(doc)
, renderer(renderer)
{
}

bool Window::handle_event(const SDL_Event &e)
{
	if (handle_button_event(e)) return true;

	switch (e.type)
	{
		case SDL_WINDOWEVENT: switch (e.window.event)
		{
			case SDL_WINDOWEVENT_CLOSE:
				if (e.window.windowID != SDL_GetWindowID(window)) break;
				quit = true;
				return true;
			case SDL_WINDOWEVENT_SHOWN:
			case SDL_WINDOWEVENT_EXPOSED:
			case SDL_WINDOWEVENT_RESTORED:
				redraw();
				return true;
		}
		return false;

		case SDL_KEYDOWN:
		case SDL_KEYUP:
			return handle_key(e.key.keysym, e.type == SDL_KEYUP);

		case SDL_MOUSEBUTTONDOWN:
		{
			if (va || e.button.button != SDL_BUTTON_LEFT) return true;
			SDL_Keymod m = SDL_GetModState();
			bool shift   = (m & (KMOD_LSHIFT|KMOD_RSHIFT));
			bool ctrl    = (m & (KMOD_LCTRL|KMOD_RCTRL));
			bool alt     = (m & (KMOD_LALT|KMOD_RALT));

			Tool t = doc.tool;
			if (t == Tool::NONE)
			{
				if (alt && !shift && !ctrl) t = Tool::SHOVEL;
				else if (ctrl) t = Tool::HIDE;
			}
			switch (t)
			{
				case Tool::SHOVEL:
					dragging = -1;
					break;
				case Tool::HIDE:
				{
					int d = hit_test(ScreenCoords(e.button.x, e.button.y), false);
					if (d < 0) break;
					hide_tool(doc.puzzle, d, alt||shift);
					start_animations();
					dragging = -2;
					break;
				}
				case Tool::MAGNET:
					dragging = hit_test(ScreenCoords(e.button.x, e.button.y), true);
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
					dragging = hit_test(ScreenCoords(e.button.x, e.button.y), true);
					if (dragging < 0) break;
					redraw();
					break;
			}
			return true;
		}
		case SDL_MOUSEBUTTONUP:
			if (va || e.button.button != SDL_BUTTON_LEFT) return true;
			drop();
			return true;
		case SDL_MOUSEMOTION:
		{
			if (va || dragging < -1) return true;

			auto buttons = e.motion.state;
			//static int i = 0; ++i; i %=10;
			//std::cout << i << "B " << buttons << std::endl;
			if (!(buttons & (SDL_BUTTON_LMASK|SDL_BUTTON_RMASK|SDL_BUTTON_MMASK))) return true;
			
			double dx = e.motion.xrel, dy = e.motion.yrel, dz = 0.0;
			if (dragging >= 0)
			{
				drag_tool(doc.puzzle, doc.camera, dragging, magnetized, drag_rel, ScreenCoords(e.motion.x, e.motion.y), drag_v, dx, dy);
				if (drag_v.absq() > 1e-12) start_animations();
				redraw();
				return true;
			}

			SDL_Keymod m = SDL_GetModState();
			bool shift   = (m & (KMOD_LSHIFT|KMOD_RSHIFT));
			bool ctrl    = (m & (KMOD_LCTRL|KMOD_RCTRL));
			bool alt     = (m & (KMOD_LALT|KMOD_RALT));

			drag_v.clear();

			if (doc.tool == Tool::SHOVEL || (alt && !ctrl && !shift))
			{
				shovel_tool(doc.puzzle, doc.camera, ScreenCoords(e.motion.x, e.motion.y), ScreenCoords(dx, dy));
				redraw();
				return true;
			}
			
			if (shift)
			{
				dz = absmax(-dx, dy);
				dx = dy = 0.0;
			}

			doc.camera.move(doc.camera.dconvert(ScreenCoords(-dx, -dy)));
			doc.camera.zoom(exp(-dz * 0.02));

			redraw();
			return true;
		}
		case SDL_MOUSEWHEEL:
		{
			if (va) return true;
			SDL_Keymod m = SDL_GetModState();
			bool shift   = (m & (KMOD_LSHIFT|KMOD_RSHIFT));

			if (e.wheel.preciseX != e.wheel.x || e.wheel.preciseY != e.wheel.y)
			{
				double dx = 5.0*e.wheel.preciseX, dy = -5.0*e.wheel.preciseY, dz = 0.0;

				int mx = -1, my = -1; SDL_GetMouseState(&mx, &my);

				if (shift)
				{
					dz = absmax(dx, -dy);
					dx = dy = 0.0;
				}

				doc.camera.move(doc.camera.dconvert(ScreenCoords(dx, dy)*10.0));
				doc.camera.zoom(exp(-dz * 0.02), ScreenCoords(mx, my));

				redraw();

				if (dragging >= 0)
					drag_tool(doc.puzzle, doc.camera, dragging, magnetized, drag_rel, ScreenCoords(mx, my), drag_v);
			}
			else
			{
				int mx = -1, my = -1;
				(void)SDL_GetMouseState(&mx, &my);
				double dx = -5.0*e.wheel.x, dy = -5.0*e.wheel.y;
				doc.camera.zoom(exp(-(dx+dy) * 0.02), ScreenCoords(mx, my));
				redraw();
			}
			return true;
		}
	}
	return false;
}

bool Window::handle_key(SDL_Keysym keysym, bool release)
{
	auto key = keysym.sym;
	auto m   = keysym.mod;
	constexpr int SHIFT = 1, CTRL = 2, ALT = 4;
	const int  shift = (m & (KMOD_LSHIFT|KMOD_RSHIFT)) ? SHIFT : 0;
	const int  ctrl  = (m & (KMOD_LCTRL|KMOD_RCTRL)) ? CTRL : 0;
	const int  alt   = (m & (KMOD_LALT|KMOD_RALT)) ? ALT : 0;
	const int  mods  = shift + ctrl + alt;
	(void)mods;

	if (!release) switch (key)
	{
		case SDLK_ESCAPE: GUI::Toggle(); return true;

		case SDLK_LEFT: case SDLK_RIGHT:
		case SDLK_UP:   case SDLK_DOWN:
		case SDLK_PLUS: case SDLK_MINUS:
			if (!ikeys.count(key)) ikeys[key] = 0.0;
			start_animations();
			return true;

		case SDLK_SPACE:
			if (ctrl) arrange(doc.puzzle, false);
			else reset_view(doc.puzzle, doc.camera);
			redraw();
			start_animations();
			return true;
		case SDLK_e:
			arrange(doc.puzzle, true);
			redraw();
			start_animations();
			return true;
		#ifdef DEBUG
		case SDLK_a:
			if (!va)
			{
				va.reset(new VictoryAnimation(doc.puzzle, doc.camera));
				start_animations();
			}
			return true;
		#endif
	}
	else switch (key)
	{
		case SDLK_LEFT: case SDLK_RIGHT:
		case SDLK_UP:   case SDLK_DOWN:
		case SDLK_PLUS: case SDLK_MINUS:
			ikeys.erase(key);
			return true;
	}
	return false;
}

bool Window::handle_button_event(const SDL_Event &e)
{
	switch (e.type)
	{
		case SDL_MOUSEBUTTONDOWN:
		{
			if (e.button.button != SDL_BUTTON_LEFT) return false;
			for (auto &b : doc.buttons.buttons)
			{
				if (!button_hit(b, e.button.x, e.button.y)) continue;
				clicked_button = b.index;
				return true;
			}
			return false;
		}
		case SDL_MOUSEBUTTONUP:
			if (e.button.button != SDL_BUTTON_LEFT) return false;
			for (auto &b : doc.buttons.buttons)
			{
				if (clicked_button != b.index) continue;
				if (!button_hit(b, e.button.x, e.button.y)) continue;
				button_action(b.index);
				clicked_button = -1;
				return true;
			}
			clicked_button = -1;
			return false;
		case SDL_MOUSEMOTION:
			return clicked_button >= 0;

		case SDL_KEYDOWN:
		{
			auto key = e.key.keysym.sym;
			switch (key)
			{
				case SDLK_1: case SDLK_2: case SDLK_3:
				case SDLK_4: case SDLK_5: case SDLK_6:
				case SDLK_7: case SDLK_8: case SDLK_9:
				{
					int i = key-SDLK_1;
					if (i < 0 || i >= (int)doc.buttons.buttons.size()) return false;
					button_action(doc.buttons.buttons[i].index);
					return true;
				}
			}
		}
	}
	return false;
}

#endif
