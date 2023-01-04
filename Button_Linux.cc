#ifdef LINUX
#include "Buttons.h"
#include "data.h"
#include "shaders.h"
#include "Utility/GL_Util.h"
#include "Utility/Preferences.h"
#include "Window.h"

static int clicked_button = -1;

bool Buttons::handle_event(const SDL_Event &e)
{
	switch (e.type)
	{
		case SDL_MOUSEBUTTONDOWN:
		{
			if (e.button.button != SDL_BUTTON_LEFT) return false;
			for (auto &b : buttons)
			{
				if (!b.hit(window, e.button.x, e.button.y)) continue;
				clicked_button = b.index;
				return true;
			}
			return false;
		}
		case SDL_MOUSEBUTTONUP:
			if (e.button.button != SDL_BUTTON_LEFT) return false;
			for (auto &b : buttons)
			{
				if (clicked_button != b.index) continue;
				if (!b.hit(window, e.button.x, e.button.y)) continue;
				window.button_action(b.index);
				clicked_button = -1;
				return true;
			}
			clicked_button = -1;
			return false;
		case SDL_MOUSEMOTION:
		{
			return clicked_button >= 0;
;
			//auto buttons = e.motion.state;
			//static int i = 0; ++i; i %=10;
			//std::cout << i << "B " << buttons << std::endl;
			//if (!(buttons & (SDL_BUTTON_LMASK|SDL_BUTTON_RMASK|SDL_BUTTON_MMASK))) return true;
			
			//double dx = e.motion.xrel, dy = e.motion.yrel, dz = 0.0;
				//doc.drag(dragging, drag_rel, e.motion.x, e.motion.y, drag_v, dx, dy);
		}

		case SDL_KEYDOWN:
		{
			auto key = e.key.keysym.sym;
			switch (key)
			{
				case SDLK_1: case SDLK_2: case SDLK_3:
				case SDLK_4: case SDLK_5: case SDLK_6:
				case SDLK_7: case SDLK_8: case SDLK_9:
				{
					int i = key-SDLK_1;
					if (i < 0 || i >= (int)buttons.size()) return false;
					window.button_action(buttons[i].index);
					return true;
				}
			}
		}
	}
	return false;
}

#endif