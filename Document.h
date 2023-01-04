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

	int hit_test(const ScreenCoords &p, bool pick_up, PuzzleCoords &rel);

	void drag(Puzzle::Piece piece, std::set<Puzzle::Piece> &magnet, const PuzzleCoords &rel, const ScreenCoords &dst, P2d &v, double mdx = 0, double mdy = 0);
	bool drop(Puzzle::Piece piece, std::set<Puzzle::Piece> &magnet);

	inline bool drop(Puzzle::Piece piece) { std::set<Puzzle::Piece> dummy; return drop(piece, dummy); }
	inline void drag(Puzzle::Piece piece, const PuzzleCoords &rel, const ScreenCoords &dst, P2d &v, double mdx = 0, double mdy = 0)
	{
		std::set<Puzzle::Piece> dummy;
		drag(piece, dummy, rel, dst, v, mdx, mdy);
	}
	
	void reset_view();
	void arrange(bool edge);
	void hide(Puzzle::Piece piece, bool and_similar = false);
	void shovel(const ScreenCoords &dst, const ScreenCoords &delta);

	void drag_view(const ScreenCoords &d);
	void zoom(float factor, const ScreenCoords &center);

	Puzzle      puzzle;
	Camera      camera;
	GL_Color    bg;
	GL_Image    im;
	std::string im_path;

private:
	void init();
	void free_image_data();
	std::unique_ptr<Histogram> histo;
};

