#pragma once
#include "imgui.h"
#include "Document.h"
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
	GUI(SDL_Window* window, SDL_GLContext context, Document &doc);
	bool handle_event(const SDL_Event &event);
	#else
	GUI(ANativeWindow *window, Document &doc);
	bool handle_touch(int ds, int n, int *id, float *x, float *y);
	#endif
	~GUI();

	enum Page
	{
		PREFERENCES,
		SETTINGS, // puzzle settings (N, cropping, ...)
	};
	void show(Page p)
	{
		if (!visible || page != p) doc.redraw(3);
		visible = true;
		page = p; init_page();
	}
	static void Show(Page p) { if (gui) gui->show(p); }

	void toggle();
	void close();
	static void Toggle() { if (gui) gui->toggle(); }

	void draw();

	static GUI *gui; // the one and only instance

private:
	static bool visible; // static to keep this alive through Android's reinit
	static Page page;
	Document &doc;

	#ifdef ANDROID
	ANativeWindow *window = NULL;
	#endif

	#ifdef DEBUG
	bool show_demo_window = false;
	#endif

	void p_preferences();

	void p_settings();
	static double tmp_N;

	void init_page();
};
