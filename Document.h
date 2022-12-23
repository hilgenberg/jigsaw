#pragma once
#include "Puzzle.h"
#include "Camera.h"
#include "Utility/GL_Image.h"
#include "Utility/GL_Color.h"
#include <string>
#include "Persistence/Serializer.h"
class Histogram;

struct Document : public Serializable
{
	Document();
	~Document();

	bool load(const std::string &im_path, int n = -1);
	void load(Deserializer &s);
	void save(Serializer &s) const;

	void draw();

	int hit_test(int mx, int my, bool pick_up, P2f &rel);

	void drag(int piece, const P2f &rel, int mx, int my, P2d &v, double mdx = 0, double mdy = 0);
	bool drop(int piece);
	
	void reset_view();
	void arrange();
	void arrange_edges();
	void hide(int piece, bool and_similar = false);
	void shovel(int mx, int my, double dx, double dy);

	Puzzle      puzzle;
	Camera      camera;
	GL_Color    bg;
	GL_Image    im;
	std::string im_path;

private:
	void init();
	void free_all();
	std::unique_ptr<Histogram> histo;
};

