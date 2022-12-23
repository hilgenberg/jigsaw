#pragma once
#include "Document.h"
#include "Buttons.h"
#include "Utility/FPSCounter.h"
#include "Victory.h"
#include <map>
#include <SDL_events.h>

class Window 
{
public:
	Window(SDL_Window* window, Document &doc);

	bool needs_redraw() const{ return need_redraw; }
	bool animating() const{ return tnf > 0.0; }
	int  current_fps() const;

	void animate();
	void draw();
	void waiting() { fps.pause(); }
	void redraw(){ need_redraw = true; }

	void reshape(int w, int h);
	bool handle_event(const SDL_Event &event);
	bool handle_key(SDL_Keysym key, bool release);

private:
	Document   &doc;
	Buttons     buttons;
	SDL_Window *window;
	int         w, h;

	double      tnf;        // scheduled time for next frame
	double      last_frame; // time of last animate() call
	FPSCounter  fps;
	bool        need_redraw;

	int         dragging = -1;
	P2d         drag_v; // move camera while dragging piece to the edge?
	P2f         drag_rel; // where is the mouse inside the dragged piece?

	std::unique_ptr<VictoryAnimation> va;
	std::map<SDL_Keycode, double> ikeys; // pressed key -> inertia
	void start_animations();
};

