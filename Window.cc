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

#ifdef LINUX
extern volatile bool quit;
extern void toggle_gui();
static inline double absmax(double a, double b){ return fabs(a) > fabs(b) ? a : b; }
#endif

void Window::start_animations() { if (tnf <= 0.0) last_frame = tnf = now(); }

void Window::animate()
{
	if (tnf <= 0.0) return;

	double t = now();
	#ifdef LINUX
	while (t < tnf)
	{
		sleep(tnf - t);
		t = now();
		if (quit) return;
	}
	#endif

	int fps = Preferences::fps();
	tnf = (fps <= 0) ? t : t + 0.99 / fps;

	double dt = std::min(std::max(1e-42, t - last_frame), 0.25);
	last_frame = t;

	if (va)
	{
		va->run(dt);
		if (!va->done()) { redraw(); return; }
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

	redraw();

	if (dragging >= 0)
	{
		// keep the piece where it is, relative to the mouse cursor
		PuzzleCoords rel_tmp = drag_rel;
		P2d v_tmp = drag_v;
		//int mx = -1, my = -1; SDL_GetMouseState(&mx, &my);
		doc.drag(dragging, magnetized, rel_tmp, dp0, v_tmp);
	}

	doc.puzzle.animate(dt);

	if (
		#ifdef LINUX
		ikeys.empty() && 
		#endif
		(dragging < 0 || drag_v.absq() < 1e-12) && doc.puzzle.animations.empty())
	{
		tnf = -1.0;
		return;
	}
}

void Window::button_action(ButtonAction a)
{
	switch (a)
	{
		case ARRANGE:      doc.arrange(false); start_animations(); break;
		case EDGE_ARRANGE: doc.arrange(true);  start_animations(); break;
		case RESET_VIEW:   doc.reset_view(); break;
		case SETTINGS:
			#ifdef LINUX
			toggle_gui();
			#endif
			break;
		case HIDE:   tool = (tool==Tool::HIDE   ? Tool::NONE : Tool::HIDE);   break;
		case SHOVEL: tool = (tool==Tool::SHOVEL ? Tool::NONE : Tool::SHOVEL); break;
		case MAGNET: tool = (tool==Tool::MAGNET ? Tool::NONE : Tool::MAGNET); break;
		default: return;
	}
	redraw();
}

void Window::reshape(int w_, int h_)
{
	if (w == w_ && h == h_) return;
	w = w_; h = h_;
	buttons.reshape();
	if (w <= 0 || h <= 0) return;
	glViewport(0, 0, w, h);
	doc.camera.viewport(w, h);
	redraw();
}

void Window::draw()
{
	need_redraw = false;
	fps.frame();
	doc.draw();
	buttons.draw();
}

int Window::current_fps() const
{
	return animating() ? (int)std::round(fps.fps()) : -1;
}
