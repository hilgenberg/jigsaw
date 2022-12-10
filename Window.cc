#include "Window.h"
#include "Document.h"
#include "Camera.h"
#include "Utility/StringFormatting.h"
#include "Utility/Preferences.h"
#include "Utility/GL_Color.h"
#include "Utility/GL_Util.h"
#include "Utility/Timer.h"
#include "Utility/Preferences.h"
#include <GL/gl.h>
#include <cassert>
#include <algorithm>
#include <SDL.h>
extern const std::vector<unsigned char> &click_data();
extern volatile bool quit;

static Uint8 *audio_buf = NULL;
static Uint32 audio_len = 0;
static SDL_AudioDeviceID audio_device = 0;
static int audio_pos = 0;

static void sfx(void *unused, Uint8 *buf, int len)
{
	//printf("sfx %d %d\n", audio_pos, len);
	int n = audio_pos < audio_len ? std::min(len, (int)(audio_len-audio_pos)) : 0;
	memcpy(buf, audio_buf+audio_pos, n);
	audio_pos += n;
	memset(buf+n, 0, len-n);
	if (audio_pos >= audio_len) SDL_PauseAudioDevice(audio_device, true);
}

static void play_click()
{
	audio_pos = 0;
	SDL_PauseAudioDevice(audio_device, false);
}

static inline double absmax(double a, double b){ return fabs(a) > fabs(b) ? a : b; }

Window::Window(SDL_Window* window, Document &doc)
: tnf(-1.0)
, last_frame(-1.0)
, w(0), h(0)
, need_redraw(true)
, window(window)
, doc(doc)
, drag_v(0.0, 0.0)
{
	auto &sd = click_data();
	SDL_RWops *io = SDL_RWFromConstMem((const void*)sd.data(), (int)sd.size());
	SDL_AudioSpec as; SDL_zero(as);
	if (!SDL_LoadWAV_RW(io, true/*free io*/, &as, &audio_buf, &audio_len))
		fprintf(stderr, "loading audio failed!\n");

	as.samples = 1024;
	as.callback = sfx;
	SDL_AudioSpec as2; SDL_zero(as2);
	audio_device = SDL_OpenAudioDevice(NULL, 0, &as, &as2, SDL_AUDIO_ALLOW_ANY_CHANGE);
}
Window::~Window()
{
	SDL_CloseAudioDevice(audio_device);
	SDL_FreeWAV(audio_buf);
}

void Window::start_animations() { if (tnf <= 0.0) last_frame = tnf = now(); }

void Window::animate()
{
	if (tnf <= 0.0) return;

	double t = now();
	while (t < tnf)
	{
		sleep(tnf - t);
		t = now();
		if (quit) return;
	}

	int fps = Preferences::fps();
	tnf = (fps <= 0) ? t : t + 0.99 / fps;

	double dt = std::min(std::max(1e-42, t - last_frame), 0.25);
	last_frame = t;

	double dx = 0.0, dy = 0.0, dz = 0.0;

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

	double f = dt * 400.0;
	dx *= f; dy *= f; dz *= f;

	SDL_Keymod m = SDL_GetModState();
	bool shift   = (m & (KMOD_LSHIFT|KMOD_RSHIFT));
	
	if (shift)
	{
		dz = absmax(dx, dy);
		dx = dy = 0.0;
	}

	if (dragging >= 0)
	{
		dx -= drag_v.x*f*3.0;
		dy -= drag_v.y*f*3.0;
	}
	doc.camera.translate(dx, dy, dz);
	redraw();

	if (dragging >= 0)
	{
		int mx = -1, my = -1; SDL_GetMouseState(&mx, &my);
		doc.drag(dragging, drag_rel, mx, my, drag_v);
	}

	doc.puzzle.animate(dt);

	if (ikeys.empty() && (dragging < 0 || drag_v.absq() < 1e-12) && doc.puzzle.animations.empty())
	{
		tnf = -1.0;
		return;
	}
}

bool Window::handle_event(const SDL_Event &e)
{
	switch (e.type)
	{
		case SDL_WINDOWEVENT:
			switch (e.window.event)
			{
				case SDL_WINDOWEVENT_CLOSE:
					if (e.window.windowID == SDL_GetWindowID(window))
					{
						// TODO: get SDL to ask before closing, so we can
						// confirm if file was changed!
						quit = true;
						return true;
					}
					break;
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
			if (e.button.button != SDL_BUTTON_LEFT) return true;
			SDL_Keymod m = SDL_GetModState();
			bool shift   = (m & (KMOD_LSHIFT|KMOD_RSHIFT));
			bool ctrl    = (m & (KMOD_LCTRL|KMOD_RCTRL));
			bool alt     = (m & (KMOD_LALT|KMOD_RALT));

			if (alt && !shift && !ctrl)
			{
				dragging = -1;
				return true;
			}

			dragging = doc.hit_test(e.button.x, e.button.y, true, drag_rel);
			if (dragging >= 0 && ctrl)
			{
				doc.hide(dragging, alt||shift);
				start_animations();
				dragging = -2;
			}
			redraw();
			return true;
		}
		case SDL_MOUSEBUTTONUP:
			if (e.button.button != SDL_BUTTON_LEFT) return true;
			if (dragging >= 0)
			{
				bool c = doc.drop(dragging);
				if (c) play_click();
			}
			dragging = -1; drag_v.clear();
			redraw();
			return true;
		case SDL_MOUSEMOTION:
		{
			if (dragging < -1) return true;

			auto buttons = e.motion.state;
			//static int i = 0; ++i; i %=10;
			//std::cout << i << "B " << buttons << std::endl;
			if (!(buttons & (SDL_BUTTON_LMASK|SDL_BUTTON_RMASK|SDL_BUTTON_MMASK))) return true;
			
			double dx = e.motion.xrel, dy = e.motion.yrel, dz = 0.0;
			if (dragging >= 0)
			{
				doc.drag(dragging, drag_rel, e.motion.x, e.motion.y, drag_v, dx, dy);
				if (drag_v.absq() > 1e-12) start_animations();
				redraw();
				return true;
			}

			SDL_Keymod m = SDL_GetModState();
			bool shift   = (m & (KMOD_LSHIFT|KMOD_RSHIFT));
			bool ctrl    = (m & (KMOD_LCTRL|KMOD_RCTRL));
			bool alt     = (m & (KMOD_LALT|KMOD_RALT));

			drag_v.clear();

			if (alt && !ctrl && !shift)
			{
				doc.shovel(e.motion.x, e.motion.y, dx, dy);
				redraw();
				return true;
			}
			
			if (shift)
			{
				dz = absmax(-dx, dy);
				dx = dy = 0.0;
			}

			doc.camera.translate(-dx, -dy, dz);
			redraw();
			return true;
		}
		case SDL_MOUSEWHEEL:
		{
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

				doc.camera.translate(10.0*dx, 10.0*dy, dz, mx, my);
				redraw();

				if (dragging >= 0)
					doc.drag(dragging, drag_rel, mx, my, drag_v);
			}
			else
			{
				int mx = -1, my = -1;
				(void)SDL_GetMouseState(&mx, &my);
				double dx = -5.0*e.wheel.x, dy = -5.0*e.wheel.y;

				doc.camera.translate(0, 0, dy, mx, my);
				doc.camera.translate(0, 0, dx, mx, my);
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
		case SDLK_LEFT: case SDLK_RIGHT:
		case SDLK_UP:   case SDLK_DOWN:
		case SDLK_PLUS: case SDLK_MINUS:
			if (!ikeys.count(key)) ikeys[key] = 0.0;
			start_animations();
			return true;
	
		case SDLK_0: case SDLK_1: case SDLK_2: case SDLK_3: case SDLK_4:
		case SDLK_5: case SDLK_6: case SDLK_7: case SDLK_8: case SDLK_9:
			if (!keys.count(key)) redraw();
			keys.insert(key);
			return true;
		
		case SDLK_SPACE:
			if (ctrl) doc.arrange();
			else doc.reset_view();
			redraw();
			start_animations();
			break;
		case SDLK_e:
			doc.arrange_edges();
			redraw();
			start_animations();
			break;
	}
	else switch (key)
	{
		case SDLK_LEFT: case SDLK_RIGHT:
		case SDLK_UP:   case SDLK_DOWN:
		case SDLK_PLUS: case SDLK_MINUS:
			ikeys.erase(key);
			return true;
	
		case SDLK_0: case SDLK_1: case SDLK_2: case SDLK_3: case SDLK_4:
		case SDLK_5: case SDLK_6: case SDLK_7: case SDLK_8: case SDLK_9:
			if (keys.count(key)) redraw();
			keys.erase(key);
			return true;
	}
	return false;
}

void Window::reshape(int w_, int h_)
{
	if (w == w_ && h == h_) return;
	w = w_; h = h_;
	if (w <= 0 || h <= 0) return;
	glViewport(0, 0, w, h);
	doc.camera.viewport(w, h);
	redraw();
}

void Window::draw()
{
	fps.frame();
	doc.draw();
	need_redraw = false;
}
int Window::current_fps() const
{
	return animating() ? (int)std::round(fps.fps()) : -1;
}
