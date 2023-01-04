#pragma once
#include "GL_Image.h"
#include <array>

class Histogram
{
public:
	typedef int Piece;
	Histogram(const GL_Image &im, int W, int H);
	double distance(Piece i, Piece j) const; // [0,1]
	double similarity(Piece i, Piece j) const; // [0,1]

private:
	std::vector<std::array<unsigned,64>> hist;
	std::vector<unsigned> total;
	std::vector<double>   unit;
};
