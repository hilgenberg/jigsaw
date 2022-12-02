#pragma once
#include <SDL.h>
#include "imgui/imgui.h"
class PlotWindow;

struct EnableGuard
{
	EnableGuard() : enabled(true) {}
	~EnableGuard() { if (!enabled) ImGui::EndDisabled(); }
	void operator()(bool e)
	{
		if (e) { if (!enabled) ImGui::EndDisabled();   } 
		else   { if ( enabled) ImGui::BeginDisabled(); }
		enabled = e;
	}
	[[nodiscard]] operator bool() const { return enabled; }
private:
	bool enabled;
};

class GUI
{
public:
	friend struct GUI_ViewMenu;
	friend struct GUI_FileMenu;

	GUI(SDL_Window* window, SDL_GLContext context, PlotWindow &w);
	~GUI();

	void show() { if (!visible) redraw(); visible = true; }
	void redraw(int n_frames = 3){ need_redraw = std::max(n_frames, need_redraw); }
	bool needs_redraw() const{ return visible && need_redraw > 0; }
	
	bool handle_event(const SDL_Event &event);
	void update(); // call this first (before waiting for the next frame)
	void draw(); // call this second

	void error(const std::string &msg);
	void confirm(bool need_confirmation, const std::string &msg, std::function<void(void)> action);

	PlotWindow &w;

private:
	bool visible;
	int  need_redraw; // imgui assumes a continuous render loop, but we only draw if
	                  // we have to, so this is some number of frames and not a single
	                  // bool to allow it to run its animations

	bool show_prefs_panel = false;
	#ifdef DEBUG
	bool show_demo_window = false;
	#endif
	
	void  prefs_panel();
	
	void error_panel();
	std::string error_msg;

	void confirmation_panel();
	std::string confirm_msg;
	std::function<void(void)> confirm_action;
};
