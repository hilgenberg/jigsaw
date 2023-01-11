#pragma once
#include "Utility/Vector.h"
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
	N_BUTTON_IMAGES
};

struct Buttons
{
	void reshape(Camera &camera);

	struct Button
	{
		ButtonAction index;
		P2f          pos; // position of center
	};
	std::vector<Button> buttons;
	P3f button_size {0, 0, 0}; // (w,h,spacing)
};
