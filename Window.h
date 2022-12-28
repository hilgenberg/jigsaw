#pragma once
#include "Document.h"
#include "Buttons.h"
#include "Utility/FPSCounter.h"
#include "Victory.h"
#include <map>
#include <SDL_events.h>

enum class Tool
{
	NONE,
	HIDE,
	SHOVEL,
	MAGNET
};

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
	P2<int> size() const { return P2<int>(w,h); }
	Tool active_tool() const { return tool; }

private:
	friend class Buttons;
	friend class GUI;
	void button_action(ButtonAction a);

	Document   &doc;
	Buttons     buttons;
	SDL_Window *window;
	int         w, h;
	Tool        tool = Tool::NONE;

	double      tnf;        // scheduled time for next frame
	double      last_frame; // time of last animate() call
	FPSCounter  fps;
	bool        need_redraw;

	int         dragging = -1;
	P2d         drag_v; // move camera while dragging piece to the edge?
	P2f         drag_rel; // where is the mouse inside the dragged piece?
	std::set<Puzzle::Piece> magnetized;

	std::unique_ptr<VictoryAnimation> va;
	std::map<SDL_Keycode, double> ikeys; // pressed key -> inertia
	void start_animations();
};

