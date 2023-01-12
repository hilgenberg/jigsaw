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
	bool load(int N); // re-dimension the puzzle (like load(same file, N))
	void load(Deserializer &s);
	void save(Serializer &s) const;

	bool animating() const{ return !puzzle.animations.empty(); }
	void draw() { if (need_redraw > 0) --need_redraw; }
	void redraw(int n_frames = 1){ need_redraw = std::max(n_frames, need_redraw); }
	bool needs_redraw() const{ return need_redraw > 0; }

	ImagePuzzle puzzle;
	Camera      camera;
	Buttons     buttons;
	Tool        tool = Tool::NONE;

private:
	int need_redraw = 1; // imgui assumes a continuous render loop, but we only draw if
	                     // we have to, so this is some number of frames and not a single
	                     // bool to allow it to run its animations

};

