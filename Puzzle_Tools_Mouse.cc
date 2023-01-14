#include "Puzzle_Tools.h"
#include "ImagePuzzle.h"
#include "Camera.h"
#include "Utility/Preferences.h"

//-----------------------------------------------------------------------------
// Hide Tool
//-----------------------------------------------------------------------------

bool is_hidden(const Puzzle &puzzle, Puzzle::Piece piece)
{
	double R = 1.25*hypot(puzzle.W, puzzle.H);
	assert(piece >= 0 && piece < puzzle.N);
	P2d p = puzzle.pos[piece];
	p.x += 0.5; p.x *= puzzle.sx;
	p.y += 0.5; p.y *= puzzle.sy;
	return p.absq() >= R*R;
}

static void hide(Puzzle &puzzle, Puzzle::Piece piece)
{
	double R = 1.25*hypot(puzzle.W, puzzle.H);
	double x = rand01(); R += 5.0*x*x;
	assert(piece >= 0 && piece < puzzle.N);
	P2d p = puzzle.pos[piece];
	p.x += 0.5; p.x *= puzzle.sx;
	p.y += 0.5; p.y *= puzzle.sy;
	p.to_unit();
	if (!std::isfinite(p.x)) p.set(-1.0, 0.0);
	p *= R;
	P2d q = puzzle.to_camera(p);
	q.x -= 0.5;
	q.y -= 0.5;
	puzzle.move(piece, q, true);
}

void hide_tool(ImagePuzzle &puzzle, Puzzle::Piece piece, bool and_similar)
{
	if (piece < 0 || piece >= puzzle.N) return;
	if (!and_similar)
	{
		::hide(puzzle, piece);
		return;
	}
	auto &histo = puzzle.histogram();
	for (int i = 0; i < puzzle.N; ++i)
	{
		if (!puzzle.should_arrange(i)) continue;
		//if (histo.distance(piece, i) < 0.2) ::hide(puzzle, i);
		if (histo.similarity(piece, i) > 0.7) ::hide(puzzle, i);
	}
}

//-----------------------------------------------------------------------------
// Drag/Magnet Tool
//-----------------------------------------------------------------------------

void drag_tool(Puzzle &puzzle, Camera &camera, Puzzle::Piece piece, std::set<Puzzle::Piece> &magnet, const PuzzleCoords &rel, const ScreenCoords &p, P2d &v, double mdx, double mdy)
{
	int w = camera.screen_w(), h = camera.screen_h();
	int pn = std::min(w, h) / 10; // size of border that causes scrolling
	P2d v0 = v;
	v.clear();
	if (p.x <= pn) v.x = (double)(pn-p.x)/pn; else if (p.x >= w-1-pn) v.x = -(double)(p.x-(w-1-pn))/pn;
	if (p.y <= pn) v.y = (double)(pn-p.y)/pn; else if (p.y >= h-1-pn) v.y = -(double)(p.y-(h-1-pn))/pn;
	if (mdx >= 0) v.x = std::min(v0.x, v.x);
	if (mdx <= 0) v.x = std::max(v0.x, v.x);
	if (mdy >= 0) v.y = std::min(v0.y, v.y);
	if (mdy <= 0) v.y = std::max(v0.y, v.y);

	if (piece < 0 || piece >= puzzle.N) return;
	auto q = puzzle.from_camera(camera.from_screen(p)) - rel;
	assert(magnet.empty() || magnet.count(piece));
	if (magnet.empty())
	{
		puzzle.move(piece, q, false);
	}
	else
	{
		assert(magnet.count(piece));
		PuzzleCoords delta = q - puzzle.pos[piece];
		puzzle.magnetize(magnet, delta);
	}
}
bool drag_tool_drop(Puzzle &puzzle, Camera &camera, Puzzle::Piece piece, std::set<Puzzle::Piece> &magnet)
{
	if (piece < 0 || piece >= puzzle.N) return false;
	auto delta = std::max(5.0*camera.pixel_size(), 0.3);
	return magnet.empty() ? puzzle.drop(piece, delta) : puzzle.drop(magnet, delta);
}

//-----------------------------------------------------------------------------
// Shovel Tool
//-----------------------------------------------------------------------------

void shovel_tool(Puzzle &puzzle, Camera &camera, const ScreenCoords &dst, const ScreenCoords &delta)
{
	CameraCoords c = camera.from_screen(dst), v = camera.dconvert(delta), v0 = v;
	if (v.absq() < 1e-18) return;
	v0.to_unit();
	double r = camera.R * 0.25;

	std::vector<int> P;
	for (int i = 0; i < puzzle.N; ++i)
	{
		if (!puzzle.should_arrange(i)) continue;
		CameraCoords p = puzzle.to_camera(puzzle.pos[i]);
		if ((p-c).absq() > r*r) continue;
		P.push_back(i);
	}
	std::sort(P.begin(), P.end(), [&puzzle, &v0, &c, r](int a, int b)
	{
		CameraCoords p1 = puzzle.to_camera(puzzle.pos[a]);
		CameraCoords p2 = puzzle.to_camera(puzzle.pos[b]);
		double y1 = (p1-c)*v0; assert(fabs(y1) <= r);
		double y2 = (p2-c)*v0; assert(fabs(y2) <= r);
		return y1 > y2;
	});

	for (int i = 0, n = (int)P.size(); i < n; ++i)
	{
		CameraCoords p = puzzle.to_camera(puzzle.pos[P[i]]);
		const double y = (p-c)*v0;
		const double x = (p-c)*CameraCoords(-v0.y, v0.x);

		// find the closest other piece behind it
		bool done = false;
		for (int j = i+1; j < n; ++j)
		{
			CameraCoords q = puzzle.to_camera(puzzle.pos[P[j]]);
			const double yj = (q-c)*v0;
			const double xj = (q-c)*CameraCoords(-v0.y, v0.x);
			assert(yj <= y);
			if (yj < y-1.0) break;
			if (fabs(xj - x) > 1.0) continue;

			p += v + v0 * ((1.0+yj-y)*0.2);
			p += CameraCoords(-v0.y, v0.x)*(0.04*(x-xj));
	
			done = true;
			break;
		}
		if (!done)
		{
			double f = -y/r / sqrt(1.00000000001 - std::min(1.0, x*x/(r*r))); // 1..-1
			f += 1.0; f /= 2.0; // 1..0
			p += v*f;
		}
		puzzle.move(P[i], puzzle.from_camera(p), false);
	}
}
