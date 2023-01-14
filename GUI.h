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
	#endif

	enum Page
	{
		PREFERENCES,
		SETTINGS, // puzzle settings (N, cropping, ...)
		DIALOG // talking to the sales creature
	};
	void show(Page p)
	{
		if (!visible || page != p) doc.redraw(3);
		visible = true;
		page = p; init_page();
	}

	void toggle();
	void close();

	void draw();
	bool visible = false; // static to keep this alive through Android's reinit

private:
	Page page = PREFERENCES;
	Document &doc;

	#ifdef DEBUG
	bool show_demo_window = false;
	#endif

	void p_preferences();

	void p_settings();
	double tmp_N = 0.0;

	void p_dialog();
	int dlg = 0;

	void init_page();

};
