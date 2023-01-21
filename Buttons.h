#pragma once
#include "Utility/Vector.h"
#include "Coordinates.h"
class Camera;

enum ButtonAction
{
	ARRANGE = 0,
	EDGE_ARRANGE,
	RESET_VIEW,
	HIDE,
	SHOVEL,
	MAGNET,
	CHANGE_IMAGE,
	SETTINGS,
	PREFERENCES,
	HELP,
	N_BUTTON_IMAGES
};

struct Buttons
{
	void reshape(Camera &camera);

	struct Button
	{
		ButtonAction index;
		ScreenCoords pos; // position of center
		const char  *help() const;
	};
	std::vector<Button> buttons;
	P3f button_size {0, 0, 0}; // (w,h,spacing)
};
