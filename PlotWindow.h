#pragma once
#include "Document.h"
#include "OpenGL/GL_RM.h"
#include "Utility/FPSCounter.h"
#include <map>
#include <SDL_events.h>

class PlotWindow 
{
public:
	PlotWindow(SDL_Window* window, GL_Context &context, Document &doc);
	virtual ~PlotWindow();

	bool needs_redraw() const{ return need_redraw; }
	bool animating() const{ return tnf > 0.0; }

	void animate();
	void draw();
	void waiting() { fps.pause(); }
	void redraw(){ need_redraw = true; }

	void reshape(int w, int h);
	bool handle_event(const SDL_Event &event);
	bool handle_key(SDL_Keysym key, bool release);

	void start_animations(); // call after Parameter::anim_start()

protected:
	Document   &doc;
	SDL_Window *window;
	int         w, h;

	double      tnf;        // scheduled time for next frame
	double      last_frame; // time of last animate() call
	GL_RM       rm;
	FPSCounter  fps;
	bool        need_redraw;
	int         dragging = -1;
	P2d         drag_v; // move camera while dragging piece to the edge?
	P2f         drag_rel; // where is the mouse inside the dragged piece?

	std::map<SDL_Keycode, double> ikeys; // pressed key -> inertia
	std::set<SDL_Keycode> keys; // pressed keys

	void translate(double dx, double dy, double dz, int mx = -1, int my = -1);
};

