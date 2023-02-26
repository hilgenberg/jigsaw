#include "Window.h"
#include "Document.h"
#include "Camera.h"
#include "Audio.h"
#include "Utility/StringFormatting.h"
#include "Utility/Preferences.h"
#include "Utility/GL_Util.h"
#include "Utility/Timer.h"
#include "Victory.h"
#include "Puzzle_Tools.h"
#include "GUI.h"

#ifdef LINUX
extern volatile bool quit;
static inline double absmax(double a, double b){ return fabs(a) > fabs(b) ? a : b; }
#else
extern void call_change_image();
#endif

void Window::start_animations() { if (!anim) { last_frame = now(); anim = true; doc.redraw(); } }

void Window::animate()
{
	if (!anim) return;

	double t = now();
	double dt = std::min(std::max(1e-42, t - last_frame), 0.25);
	last_frame = t;

	if (va)
	{
		va->run(dt);
		if (!va->done()) { doc.redraw(); return; }
		va = nullptr;
		play_click();
	}

	double dx = 0.0, dy = 0.0, dz = 0.0;
	double f = dt * 400.0;

	#ifdef LINUX
	for (auto &i : ikeys)
	{
		double &inertia = i.second;
		inertia += (3.0 + inertia*4.0)*dt;
		inertia = std::min(inertia, 5.0);
		
		switch (i.first)
		{
			case SDLK_LEFT:  dx -= inertia; break;
			case SDLK_RIGHT: dx += inertia; break;
			case SDLK_UP:    dy -= inertia; break;
			case SDLK_DOWN:  dy += inertia; break;
			case SDLK_PLUS:  dz += inertia; break;
			case SDLK_MINUS: dz -= inertia; break;
			default: assert(false); break;
		}
	}

	dx *= f; dy *= f; dz *= f;

	SDL_Keymod m = SDL_GetModState();
	bool shift   = (m & (KMOD_LSHIFT|KMOD_RSHIFT));
	
	if (shift)
	{
		dz = absmax(dx, dy);
		dx = dy = 0.0;
	}
	#endif

	ScreenCoords dp0;
	if (dragging >= 0)
	{
		dp0 = doc.camera.to_screen(doc.puzzle.to_camera(doc.puzzle.pos[dragging] + drag_rel));
		dx -= drag_v.x*f*3.0;
		dy -= drag_v.y*f*3.0;
	}

	doc.camera.move(doc.camera.dconvert(ScreenCoords(dx, dy)));
	doc.camera.zoom(exp(-dz * 0.02));

	if (dragging >= 0)
	{
		// keep the piece where it is, relative to the mouse cursor
		PuzzleCoords rel_tmp = drag_rel;
		P2d v_tmp = drag_v;
		//int mx = -1, my = -1; SDL_GetMouseState(&mx, &my);
		drag_tool(doc.puzzle, doc.camera, dragging, magnetized, rel_tmp, dp0, v_tmp);
	}

	doc.puzzle.animate(dt);

	if (
		#ifdef LINUX
		ikeys.empty() && 
		#endif
		(dragging < 0 || drag_v.absq() < 1e-12) && !doc.animating())
	{
		anim = false;
	}
	else
	{
		doc.redraw();
	}
}

void Window::play_victory_animation()
{
	if (va) return;
	va.reset(new VictoryAnimation(doc.puzzle, doc.camera));
	start_animations();
}

void Window::button_action(ButtonAction a)
{
	switch (a)
	{
		case ARRANGE:      arrange(doc.puzzle, false, true, doc.tool == Tool::HIDE); start_animations(); break;
		case EDGE_ARRANGE: arrange(doc.puzzle, true, true, doc.tool == Tool::HIDE); start_animations(); break;
		case RESET_VIEW:   reset_view(doc.puzzle, doc.camera); break;
		case CHANGE_IMAGE:
			#ifdef ANDROID
			call_change_image();
			#endif
			break;
		case SETTINGS:     gui.show(GUI::SETTINGS); break;
		case PREFERENCES:  gui.show(GUI::PREFERENCES); break;
		case HELP:         gui.show(GUI::HELP); break;
		case HIDE:         doc.tool = (doc.tool==Tool::HIDE   ? Tool::NONE : Tool::HIDE);   break;
		case SHOVEL:       doc.tool = (doc.tool==Tool::SHOVEL ? Tool::NONE : Tool::SHOVEL); break;
		case MAGNET:       doc.tool = (doc.tool==Tool::MAGNET ? Tool::NONE : Tool::MAGNET); break;
		default: assert(false); return;
	}
	doc.redraw();
}

void Window::reshape(int w, int h)
{
	auto &c = doc.camera;
	if (w == c.screen_w() && h == c.screen_h()) return;
	doc.camera.viewport(w, h);
	doc.buttons.reshape(doc.camera);
	doc.redraw();
}

int Window::hit_test(const ScreenCoords &p, bool pick_up)
{
	float r = Preferences::finger_radius();
	Puzzle::Piece i = doc.puzzle.hit_test(
		doc.puzzle.from_camera(doc.camera.from_screen(p)),
		doc.puzzle.from_camera(doc.camera.dconvert(ScreenCoords(r, r))), 
		drag_rel);
	if (pick_up && i >= 0) doc.puzzle.pick_up(i);
	return i;
}

bool Window::button_hit(const Buttons::Button &b, float mx, float my)
{
	auto &camera = doc.camera;
	int w = camera.screen_w(), h = camera.screen_h();
	float x = 2.0f*mx / w - 1.0f, y = 1.0f - 2.0f*my / h;
	return fabs(x - b.pos.x) < doc.buttons.button_size.x*0.5f &&
	       fabs(y - b.pos.y) < doc.buttons.button_size.y*0.5f;
}

void Window::drop()
{
	if (dragging < 0) { dragging = -1; return; }
	if (drag_tool_drop(doc.puzzle, doc.camera, dragging, magnetized))
	{
		play_click();
		if (doc.puzzle.solved() && !va) play_victory_animation();
	}
	dragging = -1;
	drag_v.clear();
	magnetized.clear();
	doc.redraw();
}
