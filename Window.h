#pragma once
#include "Document.h"
#include "Victory.h"
#ifdef LINUX
#include <SDL_events.h>
#endif
class GUI;

class Window 
{
public:
	#ifdef LINUX
	Window(SDL_Window* window, Document &doc, GUI &gui);
	bool handle_event(const SDL_Event &event);
	bool handle_key(SDL_Keysym key, bool release);
	bool handle_button_event(const SDL_Event &event);
	#endif

	#ifdef ANDROID
	Window(Document &doc, GUI &gui);
	// ds is  1: touch down, -1: touch lifted, 0: movement
	// special case ds == -1, n == 0: lift all / cancel gesture
	void handle_touch(int ds, int n, int *id, float *x, float *y);
	#endif

	void animate();
	void reshape(int w, int h);
	void button_action(ButtonAction a);
	void play_victory_animation();

private:
	Document     &doc;
	GUI          &gui;
	bool          anim = false;
	double        last_frame = -1.0; // time of last animate() call

	Puzzle::Piece dragging = -1;
	P2d           drag_v {0.0, 0.0}; // move camera while dragging piece to the edge?
	PuzzleCoords  drag_rel; // where is the mouse inside the dragged piece?
	std::set<Puzzle::Piece> magnetized;
	int           clicked_button = -1;

	std::unique_ptr<VictoryAnimation> va;
	void start_animations();
	int  hit_test(const ScreenCoords &p, bool pick_up);
	bool button_hit(const Buttons::Button &b, float mx, float my);
	void drop(); // release dragged piece(s)

	#ifdef LINUX
	SDL_Window *window;
	std::map<SDL_Keycode, double> ikeys; // pressed key -> inertia
	#endif

	#ifdef ANDROID
	std::map<int, ScreenCoords> pointer_state;
	#endif
};

