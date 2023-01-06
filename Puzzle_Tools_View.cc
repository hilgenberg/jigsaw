#include "Puzzle_Tools.h"
#include "Puzzle.h"
#include "Camera.h"

void reset_view(Puzzle &puzzle, Camera &camera)
{
	double x0, x1, y0, y1;
	puzzle.bbox(x0, x1, y0, y1);
	x0 = std::min(x0, -0.5*puzzle.W*puzzle.sx);
	x1 = std::max(x1,  0.5*puzzle.W*puzzle.sx);
	y0 = std::min(y0, -0.5*puzzle.H*puzzle.sy);
	y1 = std::max(y1,  0.5*puzzle.H*puzzle.sy);
	camera.view_box(x0, x1, y0, y1, 1.25);
}
