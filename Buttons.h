#pragma once
#include "Utility/GL_Image.h"
#include <SDL_events.h>
class Window;

class Buttons
{
public:
	Buttons(Window &w);
	~Buttons();
	void draw();
	bool handle_event(const SDL_Event &e);
	void reshape(int w, int h);

private:
	Window &w;
};