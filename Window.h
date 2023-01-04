#pragma once
#include "Document.h"
#include "Buttons.h"
#include "Utility/FPSCounter.h"
#include "Victory.h"
#ifdef LINUX
#include <SDL_events.h>
#endif

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
	#ifdef LINUX
	Window(SDL_Window* window, Document &doc);
	bool handle_event(const SDL_Event &event);
	bool handle_key(SDL_Keysym key, bool release);
	#endif

	#ifdef ANDROID
	Window(Document &doc);
	~Window();

	// ds ==  1: touch down
	// ds == -1: touch lifted
	// ds ==  0: movement
	// special case ds == -1, n == 0: lift all, cancel gesture
	void handle_touch(int ds, int n, int *id, float *x, float *y);
	#endif

	bool needs_redraw() const{ return need_redraw; }
	bool animating() const{ return tnf > 0.0; }
	int  current_fps() const;

	void animate();
	void draw();
	void waiting() { fps.pause(); }
	void redraw(){ need_redraw = true; }

	void reshape(int w, int h);
	
	P2<int> size() const { return P2<int>(w,h); }
	Tool active_tool() const { return tool; }

private:
	friend class Buttons;
	void button_action(ButtonAction a);

	Document   &doc;
	Buttons     buttons;
	int         w = 0, h = 0;
	Tool        tool = Tool::NONE;

	double      tnf = -1.0;        // scheduled time for next frame
	double      last_frame = -1.0; // time of last animate() call
	FPSCounter  fps;
	bool        need_redraw = true;

	Puzzle::Piece dragging = -1;
	P2d           drag_v {0.0, 0.0}; // move camera while dragging piece to the edge?
	PuzzleCoords  drag_rel; // where is the mouse inside the dragged piece?
	std::set<Puzzle::Piece> magnetized;

	std::unique_ptr<VictoryAnimation> va;
	void start_animations();

	#ifdef LINUX
	friend class GUI;
	SDL_Window *window;
	std::map<SDL_Keycode, double> ikeys; // pressed key -> inertia
	#endif

	#ifdef ANDROID
	const EGLContext context;
	std::map<int, ScreenCoords> pointer_state;
	int drag_pointer_id = -1;
	#endif
};

