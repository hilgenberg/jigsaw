#pragma once
#include "imgui.h"
class Window;
#ifdef LINUX
#include <SDL.h>
#endif
#ifdef ANDROID
struct ANativeWindow;
#endif

class GUI
{
public:
	#ifdef LINUX
	GUI(SDL_Window* window, SDL_GLContext context, Window &w);
	bool handle_event(const SDL_Event &event);
	#else
	GUI(ANativeWindow *window, Window &w);
	bool handle_touch(int ds, int n, int *id, float *x, float *y);
	#endif
	~GUI();

	enum Page
	{
		PREFERENCES,
		SETTINGS, // puzzle settings (N, cropping, ...)
	};
	void show(Page p) { if (!visible) redraw(); visible = true; page = p; init_page(); }
	static void Show(Page p) { if (gui) gui->show(p); }

	void toggle();
	void close();
	static void Toggle() { if (gui) gui->toggle(); }

	void redraw(int n_frames = 3){ need_redraw = std::max(n_frames, need_redraw); }
	bool needs_redraw() const{ return visible && need_redraw > 0; }
	
	void draw();

	static GUI *gui; // the one and only instance
	Window &w;

private:
	bool visible = false;
	Page page = PREFERENCES;
	int  need_redraw = 0; // imgui assumes a continuous render loop, but we only draw if
	                      // we have to, so this is some number of frames and not a single
	                      // bool to allow it to run its animations
	#ifdef ANDROID
	ANativeWindow *window = NULL;
	#endif

	#ifdef DEBUG
	bool show_demo_window = false;
	#endif

	void p_preferences();

	void p_settings();
	double tmp_N;

	void init_page();
};
