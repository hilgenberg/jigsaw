#include "Document.h"
#include "Utility/Histogram.h"

static void hide(Puzzle &puzzle, Puzzle::Piece piece)
{
	double R = 1.25*hypot(puzzle.W, puzzle.H);
	double x = rand01(); R += 5.0*x*x;
	assert(piece >= 0 && piece < puzzle.N);
	P2d p = (P2d)puzzle.pos[piece];
	p.x += 0.5; p.x *= puzzle.sx;
	p.y += 0.5; p.y *= puzzle.sy;
	p.to_unit();
	if (!std::isfinite(p.x)) p.set(-1.0, 0.0);
	p *= R;
	p.x -= 0.5*puzzle.sx;
	p.y -= 0.5*puzzle.sy;
	puzzle.move(piece, p.x, p.y, true);
}

void Document::hide(Puzzle::Piece piece, bool and_similar)
{
	if (piece < 0 || piece >= puzzle.N) return;
	if (and_similar)
	{
		if (!histo) histo.reset(new Histogram(im, puzzle.W, puzzle.H));
		for (int i = 0; i < puzzle.N; ++i)
		{
			if (!puzzle.should_arrange(i)) continue;
			//if (histo->distance(piece, i) < 0.2) ::hide(puzzle, i);
			if (histo->similarity(piece, i) > 0.7) ::hide(puzzle, i);
		}
	}
	else
	{
		::hide(puzzle, piece);
	}
}

int Document::hit_test(int mx, int my, bool pick_up, P2f &rel)
{
	auto p = camera.convert(mx, my);
	p.x /= puzzle.sx;
	p.y /= puzzle.sy;
	Puzzle::Piece i = puzzle.hit_test(p, rel);
	if (pick_up && i >= 0) puzzle.pick_up(i);
	return i;
}
void Document::drag(Puzzle::Piece piece, std::set<Puzzle::Piece> &magnet, const P2f &rel, int mx, int my, P2d &v, double mdx, double mdy)
{
	int w = camera.screen_w(), h = camera.screen_h();
	int pn = std::min(w, h) / 10; // size of border that causes scrolling
	P2d v0 = v;
	v.clear();
	if (mx <= pn) v.x = (double)(pn-mx)/pn; else if (mx >= w-1-pn) v.x = -(double)(mx-(w-1-pn))/pn;
	if (my <= pn) v.y = (double)(pn-my)/pn; else if (my >= h-1-pn) v.y = -(double)(my-(h-1-pn))/pn;
	if (mdx >= 0) v.x = std::min(v0.x, v.x);
	if (mdx <= 0) v.x = std::max(v0.x, v.x);
	if (mdy >= 0) v.y = std::min(v0.y, v.y);
	if (mdy <= 0) v.y = std::max(v0.y, v.y);

	if (piece < 0 || piece >= puzzle.N) return;
	auto p = camera.convert(mx, my);
	p.x /= puzzle.sx;
	p.y /= puzzle.sy;
	p -= rel;
	assert(magnet.empty() || magnet.count(piece));
	if (magnet.empty())
	{
		puzzle.move(piece, p, false);
		return;
	}
	else
	{
		assert(magnet.count(piece));
		P2f delta = p - puzzle.pos[piece];
		puzzle.magnetize(magnet, delta);
	}
}
bool Document::drop(Puzzle::Piece piece, std::set<Puzzle::Piece> &magnet)
{
	if (piece < 0 || piece >= puzzle.N) return false;
	auto delta = std::max(5.0*camera.pixel_size(), 0.3);
	return magnet.empty() ? puzzle.connect(piece, delta) : puzzle.connect(magnet, delta);
}

void Document::shovel(int mx, int my, double dx, double dy)
{
	P2f c = camera.convert(mx, my);
	P2f v = camera.dconvert(dx, dy);
	P2f v0 = v; v0.to_unit();
	double r = camera.R * 0.25;

	std::vector<int> P;
	for (int i = 0; i < puzzle.N; ++i)
	{
		if (!puzzle.should_arrange(i)) continue;
		P2f p = puzzle.get(i);
		if ((p-c).absq() > r*r) continue;
		P.push_back(i);
	}
	std::sort(P.begin(), P.end(), [this, &v0, &c, r](int a, int b)
	{
		P2f p1 = puzzle.get(a);
		P2f p2 = puzzle.get(b);
		double y1 = (p1-c)*v0; assert(fabs(y1) <= r);
		double y2 = (p2-c)*v0; assert(fabs(y2) <= r);
		return y1 > y2;
	});

	for (int i = 0, n = (int)P.size(); i < n; ++i)
	{
		P2f p = puzzle.get(P[i]);
		const double y = (p-c)*v0;
		const double x = (p-c)*P2f(-v0.y, v0.x);

		// find the closest other piece behind it
		bool done = false;
		for (int j = i+1; j < n; ++j)
		{
			P2f q = puzzle.get(P[j]);
			const double yj = (q-c)*v0;
			const double xj = (q-c)*P2f(-v0.y, v0.x);
			assert(yj <= y);
			if (yj < y-1.0) break;
			if (fabs(xj - x) > 1.0) continue;

			p += v + v0 * ((1.0f+yj-y)*0.2);
			p += P2f(-v0.y, v0.x)*(0.04*(x-xj));
	
			//q -= P2f(-v0.y, v0.x)*(0.04*(x-xj));
			//puzzle.move(P[j], q.x, q.y, false);

			done = true;
			break;
		}
		if (!done)
		{
			double f = -y/r / sqrt(1.00000000001 - std::min(1.0, x*x/(r*r))); // 1..-1
			f += 1.0; f /= 2.0; // 1..0
			p.x += f*v.x;
			p.y += f*v.y;
		}
		puzzle.move(P[i], p.x, p.y, false);
	}
}
