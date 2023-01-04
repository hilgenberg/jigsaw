#pragma once
#include "Utility/GL_Image.h"
#include "Utility/Vector.h"
#ifdef LINUX
#include <SDL_events.h>
#endif
class Window;

enum ButtonAction
{
	ARRANGE = 0,
	EDGE_ARRANGE,
	RESET_VIEW,
	HIDE,
	SHOVEL,
	MAGNET,
	SETTINGS,
	N_IMAGES
};

class Buttons
{
public:
	Buttons(Window &w);
	~Buttons();
	void draw();
	void reshape();

	#ifdef LINUX
	bool handle_event(const SDL_Event &e);
	#endif
	#ifdef ANDROID
	bool handle_touch(int ds, int n, int *id, float *x, float *y);
	#endif

private:
	Window &window;

	struct Button
	{
		ButtonAction index; // index into button texture
		P2f pos; // center position
		bool hit(const Window &w, int mx, int my) const;
	};
	std::vector<Button> buttons;
};
