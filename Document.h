#pragma once
#include "ImagePuzzle.h"
#include "Camera.h"
#include "Utility/GL_Color.h"
#include "Persistence/Serializer.h"
#include "Buttons.h"

enum class Tool
{
	NONE,
	HIDE,
	SHOVEL,
	MAGNET
};

struct Document : public Serializable
{
	Document();

	bool load(const std::string &im_path, int n = -1);
	void load(Deserializer &s);
	void save(Serializer &s) const;
	bool reset_N(int N);

	bool animating() const{ return !puzzle.animations.empty(); }
	void draw();

	ImagePuzzle puzzle;
	Camera      camera;
	Buttons     buttons;
	Tool        tool = Tool::NONE;
};

