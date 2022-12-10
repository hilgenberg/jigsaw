#pragma once
#include <SDL.h>
#include "imgui/imgui.h"
class Window;

class GUI
{
public:
	GUI(SDL_Window* window, SDL_GLContext context, Window &w);
	~GUI();

	void show() { if (!visible) redraw(); visible = true; }
	void redraw(int n_frames = 3){ need_redraw = std::max(n_frames, need_redraw); }
	bool needs_redraw() const{ return visible && need_redraw > 0; }
	
	bool handle_event(const SDL_Event &event);
	void update(); // call this first (before waiting for the next frame)
	void draw(); // call this second

	Window &w;

private:
	bool visible;
	int  need_redraw; // imgui assumes a continuous render loop, but we only draw if
	                  // we have to, so this is some number of frames and not a single
	                  // bool to allow it to run its animations

	#ifdef DEBUG
	bool show_demo_window = false;
	#endif
};
