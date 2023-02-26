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
	GUI(Document &doc) : doc(doc) {}

	#ifdef LINUX
	bool handle_event(const SDL_Event &event);
	#else
	bool handle_touch(int ds, int n, int *id, float *x, float *y);
	bool activate_secret_menu(int ds, int n, int *id, float *x, float *y);
	bool handle_back_button();
	#endif

	enum Page
	{
		PREFERENCES,
		SETTINGS, // puzzle settings (N, cropping, ...)
		SECRET_MENU,
		HELP
	};
	void show(Page p)
	{
		if (visible && p == page && p != SECRET_MENU) { close(); return; }
		if (!visible || page != p) doc.redraw(3);
		visible = true;
		page = p; init_page();
	}

	void toggle();
	void close();

	void draw();
	bool visible = false; // static to keep this alive through Android's reinit

private:
	Document &doc;
	Page page = PREFERENCES;

	#ifdef DEBUG
	bool show_demo_window = false;
	#endif

	void p_preferences();

	void p_settings();
	double tmp_N = 0.0;

	void p_secret();

	void p_help();

	void init_page();

	#ifdef ANDROID
	int liftoff = 0; // workaround for an ImGui event queue limitation
	#endif
};
