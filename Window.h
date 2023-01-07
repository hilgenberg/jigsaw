#pragma once
#include "Document.h"
#include "Victory.h"
class Renderer;
#ifdef LINUX
#include <SDL_events.h>
#endif

class Window 
{
public:
	#ifdef LINUX
	Window(SDL_Window* window, Document &doc, Renderer &renderer);
	bool handle_event(const SDL_Event &event);
	bool handle_key(SDL_Keysym key, bool release);
	bool handle_button_event(const SDL_Event &event);
	#endif

	#ifdef ANDROID
	Window(Document &doc, Renderer &renderer);
	// ds is  1: touch down, -1: touch lifted, 0: movement
	// special case ds == -1, n == 0: lift all / cancel gesture
	void handle_touch(int ds, int n, int *id, float *x, float *y, std::function<void(ButtonAction)> button_callback);
	#endif

	void redraw();
	bool animating() const{ return tnf > 0.0; }
	void animate();
	void reshape(int w, int h);

	void button_action(ButtonAction a);

private:
	Document   &doc;
	Renderer   &renderer; // just needed for calling redraw() on it

	double      tnf = -1.0;        // scheduled time for next frame
	double      last_frame = -1.0; // time of last animate() call

	Puzzle::Piece dragging = -1;
	P2d           drag_v {0.0, 0.0}; // move camera while dragging piece to the edge?
	PuzzleCoords  drag_rel; // where is the mouse inside the dragged piece?
	std::set<Puzzle::Piece> magnetized;

	std::unique_ptr<VictoryAnimation> va;
	void start_animations();
	int  hit_test(const ScreenCoords &p, bool pick_up);
	bool button_hit(const Buttons::Button &b, float mx, float my);
	void drop(); // release dragged piece(s)

	#ifdef LINUX
	friend class GUI;
	SDL_Window *window;
	std::map<SDL_Keycode, double> ikeys; // pressed key -> inertia
	int clicked_button = -1;
	#endif

	#ifdef ANDROID
	std::map<int, ScreenCoords> pointer_state;
	int clicked_button = -1;
	#endif
};

