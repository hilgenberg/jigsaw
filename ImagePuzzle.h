#pragma once
#include "Puzzle.h"
#include "Utility/GL_Image.h"
#include "Utility/Histogram.h"
#include "Persistence/Serializer.h"

struct ImagePuzzle : public Puzzle
{
	bool load(const std::string &im_path);
	void load(Deserializer &s);
	void save(Serializer &s) const;

	GL_Image    im;
	std::string im_path;

	Histogram &histogram();

private:
	std::unique_ptr<Histogram> histo;
};

