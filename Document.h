#pragma once
#include "Puzzle.h"
#include "Camera.h"
#include "OpenGL/GL_Image.h"
#include "OpenGL/GL_Color.h"
#include <string>

struct Document
{
	Document(const std::string &im_path, int n = -1);
	~Document();

	void draw();

	void load(const std::string &path);
	void saveAs(const std::string &path) const;

	int hit_test(int mx, int my, bool pick_up = true);
	void drag(int piece, int dx, int dy);
	void drop(int piece);
	
	void reset_view();
	void arrange();
	void arrange_edges();


	Puzzle   puzzle;
	Camera   camera;
	GL_Color bg;
	GL_Image im;

protected:
};

