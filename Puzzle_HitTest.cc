#include "Puzzle.h"

static inline double sqr(double x) { return x*x; }

// helper function for circle-rectangle intersection
// box of width 2rx, height 2ry, centered at origin vs circle at c with radius r
#if 0 // unused
static bool circle_vs_rect(double r, const PuzzleCoords &c, double rx, double ry)
{
	assert(rx >= 0.0 && ry >= 0.0 && r >= 0.0);
	if (fabs(c.y) < ry) return fabs(c.x) < rx+r;
	if (fabs(c.x) < rx) return fabs(c.y) < ry+r;
	return sqr(fabs(c.x)-rx)+sqr(fabs(c.y)-ry) < r*r;
}
#endif
static bool ellipse_vs_rect(const PuzzleCoords &r, const PuzzleCoords &c, double rx, double ry)
{
	//same as circle_vs_rect(1.0, P2d(c.x/r.x, c.y/r.y), rx/r.x, ry/r.y);
	if (fabs(c.y) < ry) return fabs(c.x) < rx + r.x;
	if (fabs(c.x) < rx) return fabs(c.y) < ry + r.y;
	return sqr((fabs(c.x)-rx)/r.x)+sqr((fabs(c.y)-ry)/r.y) < 1.0;
}

// helper functions from the shader code (so they stay as similar as possible):
static inline int tx(int b) { return (b & 1) - ((b>>1) & 1); }
static inline float dq(const P2d &v, double r) { return v.absq() - r*r; }
static inline float min(double a, double b) { return std::min(a,b); }
static inline float max(double a, double b) { return std::max(a,b); }

Puzzle::Piece Puzzle::hit_test(const PuzzleCoords &p, const PuzzleCoords &radius, PuzzleCoords &rel) const // any piece at p +- r?
{
	float d1, d0;
	const auto edge = Preferences::edge();
	overhang(edge, d1, d0);
	double d = std::max(d1, d0);
	const auto x = p.x, y = p.y;

	//----------------------------------------------------------------------------------
	// exact test first (this could be skipped if radius is huge, compared to the puzzle
	// pieces, but we want to prevent that by capping the zoom at sane levels)
	//----------------------------------------------------------------------------------

	for (int j = N-1; j >= 0; --j)
	{
		const int i = z[j];
		const double px = pos[i].x, py = pos[i].y;
		if (x < px-d || x > px+1.0+d || y < py-d || y > py+1.0+d) continue;
		
		const int b = (int)borders[i];
		const int bl = tx(b);
		const int br = tx(b>>2);
		const int bt = tx(b>>4);
		const int bb = tx(b>>6);

		double x0 = 0.0, x1 = 1.0, y0 = 0.0, y1 = 1.0;
		if (bl == 1) x0 -= d1; else if (bl == -1) x0 -= d0;
		if (br == 1) x1 += d1; else if (br == -1) x1 += d0;
		if (bt == 1) y0 -= d1; else if (bt == -1) y0 -= d0;
		if (bb == 1) y1 += d1; else if (bb == -1) y1 += d0;
		if (x-px < x0 || x-px > x1 || y-py < y0 || y-py > y1) continue;

		bool hit = true;
		P2d orig(x-px, y-py);
		#define discard hit=false
		#define vec2 P2d
		switch (edge)
		{
			case None: break;
			case Regular:
			{
				const double R = 1.5, r = 0.15, d0 = 0.0, h = 0.08578643762690485;
				if (bl*max(dq(vec2(    bl*(h-R), 0.5)-orig, R), -dq(vec2(   -bl*d0, 0.5)-orig, r)) < -1e-8) discard;
				if (br*max(dq(vec2(1.0+br*(R-h), 0.5)-orig, R), -dq(vec2(1.0+br*d0, 0.5)-orig, r)) < -1e-8) discard;
				if (bt*max(dq(vec2(0.5,     bt*(h-R))-orig, R), -dq(vec2(0.5,    -bt*d0)-orig, r)) < -1e-8) discard;
				if (bb*max(dq(vec2(0.5, 1.0+bb*(R-h))-orig, R), -dq(vec2(0.5, 1.0+bb*d0)-orig, r)) < -1e-8) discard;
				break;
			}
			case Triangle:
			{
				const double h1 = 0.15, h2 = 0.05, w = 0.5/3.0;
				const double x = fabs(orig.x-0.5), y = fabs(orig.y-0.5);
				const double a = (h1+h2)/w, b = h1*h2/(h1-h2);
				const double xx = min(a*x - h1, b - 2.0*b*x);
				const double yy = min(a*y - h1, b - 2.0*b*y);
				if (    orig.y < bt * xx) discard;
				if (1.0-orig.y < bb * xx) discard;
				if (    orig.x < bl * yy) discard;
				if (1.0-orig.x < br * yy) discard;
				break;
			}
			case Groove:
			{
				double h = 0.034, r = 0.2;
				if (bt*fabs(orig.x-0.5) > bt*r && orig.y <     h) discard;
				if (bb*fabs(orig.x-0.5) > bb*r && orig.y > 1.0-h) discard;
				if (bl*fabs(orig.y-0.5) > bl*r && orig.x <     h) discard;
				if (br*fabs(orig.y-0.5) > br*r && orig.x > 1.0-h) discard;
				break;
			}
			case Circle:
			{
				const double r = 0.2;
				const double x = fabs(orig.x-0.5), y = fabs(orig.y-0.5);
				if (bt*orig.y < 0.0 && bt*hypotf(x,     orig.y) > bt*r) discard;
				if (bb*orig.y > bb  && bb*hypotf(x, 1.0-orig.y) > bb*r) discard;
				if (bl*orig.x < 0.0 && bl*hypotf(y,     orig.x) > bl*r) discard;
				if (br*orig.x > br  && br*hypotf(y, 1.0-orig.x) > br*r) discard;
				break;
			}
			default: assert(false);
		}
		#undef discard
		#undef vec2

		if (hit)
		{
			rel.set(x-px, y-py);
			return i;
		}
	}

	//----------------------------------------------------------------------------------
	// try again with the full finger ellipse (but only against the inner rectangle for
	// now because the proper test would be quite complex)
	//----------------------------------------------------------------------------------

	if (radius.x < 0.01 && radius.y < 0.01) return -1; // don't bother
	if (radius.x < 1e-6 || radius.y < 1e-6) return -1; // sanity check

	for (int j = N-1; j >= 0; --j)
	{
		const int i = z[j];
		const double px = pos[i].x, py = pos[i].y;
		
		//if (!ellipse_vs_rect(radius, P2d(x-px-0.5, y-py-0.5), 0.5+d, 0.5+d)) continue;
		if (!ellipse_vs_rect(radius, P2d(x-px-0.5, y-py-0.5), 0.5, 0.5)) continue;

		#if 0
		const int b = (int)borders[i];
		const int bl = tx(b);
		const int br = tx(b>>2);
		const int bt = tx(b>>4);
		const int bb = tx(b>>6);

		double x0 = 0.0, x1 = 1.0, y0 = 0.0, y1 = 1.0;
		if (bl == 1) x0 -= d1; else if (bl == -1) x0 -= d0;
		if (br == 1) x1 += d1; else if (br == -1) x1 += d0;
		if (bt == 1) y0 -= d1; else if (bt == -1) y0 -= d0;
		if (bb == 1) y1 += d1; else if (bb == -1) y1 += d0;
		if (x-px < x0 || x-px > x1 || y-py < y0 || y-py > y1) continue;
		#endif

		rel.set(x-px, y-py);
		return i;
	}

	return -1;
}
