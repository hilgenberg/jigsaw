#include "Document.h"
#include "Utility/Preferences.h"

static inline bool even(int i) { return !(i&1); }

static void pack_edge(Puzzle &puzzle, std::vector<int> &P, P2f c, P2f r, float w, float h)
// c +- r/2 are the edge's endpoints
// w, h are the pieces' dimensions along and across the edge
{
	std::sort(P.begin(), P.end(), [&puzzle](int a, int b) { return puzzle.z[a] > puzzle.z[b]; });
	const float spcx = 0.3*w + 0.25f, spcy = 0.3*h + 0.25f;
	float W = r.abs();
	int nx = std::floor((W-spcx) / (w+spcx));
	r /= W;
	P2f s(r.y, -r.x);

	for (int n = 0; n < (int)P.size(); ++n)
	{
		int i = P[n];
		int x = n%nx + 1, y = n / nx;
		x /= 2; // 0, 1, 1, 2, 2, ...
		x *= (n&1 ? -1 : 1); // 0, -1, 1, -2, 2, ...
		P2f p = c + r * (x*(w+spcx)) + s * (y*(h+spcy));

		int nrow = std::min((int)P.size() - nx*y, nx);
		if (even(nrow)) p += r * (0.5f*(w+spcx));

		puzzle.move(i, p.x, p.y, true);
	}
}

static void move(Puzzle &puzzle, Puzzle::Piece i, float x, float y, bool anim, float &x0, float &x1, float &y0, float &y1)
{
	puzzle.move(i, x, y, anim);
	if (x < x0) x0 = x; if (x+puzzle.sx > x1) x1 = x+puzzle.sx;
	if (y < y0) y0 = y; if (y+puzzle.sy > y1) y1 = y+puzzle.sy;
}

void Document::arrange(bool edge)
{
	const int W = puzzle.W, H = puzzle.H;
	if (W < 2 || H < 2 || Preferences::edge() == None) edge = false;

	// leave space for assembled pieces
	float X0, X1, Y0, Y1;
	puzzle.bbox(X0, X1, Y0, Y1, true);
	// leave space for assembly area
	if (X0 > -0.5f*W) X0 = -0.5f*W; if (X1 < 0.5f*W) X1 = 0.5f*W;
	if (Y0 > -0.5f*H) Y0 = -0.5f*H; if (Y1 < 0.5f*H) Y1 = 0.5f*H;
	// spacing
	X0 -= 0.5; X1 += 0.5;
	Y0 -= 0.5; Y1 += 0.5;

	const float w = puzzle.sx, h = puzzle.sy;
	const bool ani = (puzzle.N < 1000000);
	if (Preferences::spiral())
	{
		const double rp = 0.5*1.3*hypot(w, h);
		P2f c(0.5f*(X0+X1)-0.5f*w, 0.5f*(Y0+Y1)-0.5f*h);
		double r = 0.5*hypot(X1-X0, Y1-Y0) + rp, a = atan2(Y0-Y1, X0-X1); //-M_PI*0.5;
		for (int i : puzzle.z)
		{
			if (!puzzle.should_arrange(i)) continue;
			if (edge && puzzle.is_edge_piece(i)) continue;

			float x = c.x + r*cos(a);
			float y = c.y + r*sin(a);
			move(puzzle, i, x, y, ani, X0, X1, Y0, Y1);
			a += 2.0*rp/r;
			r += 2.0*rp/r * rp/M_PI;
		}
	}
	else
	{
		const float spcx = 0.3*w + 0.25f, spcy = 0.3*h + 0.25f;
		int nx = std::ceil((X1-X0-spcx) / (w+spcx));
		int ny = std::ceil((Y1-Y0-spcy) / (h+spcy));
		float X00 = 0.5f*(X0+X1) - 0.5f*(nx > 0 ? nx*(w+spcx)-spcx : 0.0f);
		float Y00 = 0.5f*(Y0+Y1) - 0.5f*(ny > 0 ? ny*(h+spcy)-spcy : 0.0f);

		int x0 = 0, x1 = nx, y0 = 0, y1 = ny;
		int ix = 0, iy = -1, dx = 1, dy = 0;
		if (nx == 0) { dx = 0; dy = 1; }

		for (int i : puzzle.z)
		{
			if (!puzzle.should_arrange(i)) continue;
			if (edge && puzzle.is_edge_piece(i)) continue;
			float x = X00 + ix * (w+spcx);
			float y = Y00 + iy * (h+spcy);
			move(puzzle, i, x, y, ani, X0, X1, Y0, Y1);
			ix += dx;
			iy += dy;
			if      (dx ==  1 && ix >= x1  ) { dx =  0; dy =  1; --y0; }
			else if (dy ==  1 && iy >= y1  ) { dx = -1; dy =  0; ++x1; }
			else if (dx == -1 && ix <= x0-1) { dx =  0; dy = -1; ++y1; }
			else if (dy == -1 && iy <= y0-1) { dx =  1; dy =  0; --x0; }
		}
	}

	if (edge)
	{
		const int N = puzzle.N;
		X0 -= 2.0; X1 += 2.0;
		Y0 -= 2.0; Y1 += 2.0;
		float Xm = (X0+X1)*0.5f, Ym = (Y0+Y1)*0.5f;
		const float w = puzzle.sx, h = puzzle.sy;

		if (puzzle.should_arrange( 0 )) puzzle.move(  0, X0-w, Y0-h, true);
		if (puzzle.should_arrange(W-1)) puzzle.move(W-1, X1,   Y0-h, true);
		if (puzzle.should_arrange(N-W)) puzzle.move(N-W, X0-w, Y1,   true);
		if (puzzle.should_arrange(N-1)) puzzle.move(N-1, X1,   Y1,   true);

		std::vector<int> P; for (int i = 1; i < W-1; ++i) if (puzzle.should_arrange(i)) P.push_back(i);
		pack_edge(puzzle, P, P2f(Xm-0.5f*w, Y0-h), P2f(X1-X0, 0.0f), w, h);

		P.clear(); for (int i = W+W-1; i < N-1; i += W) if (puzzle.should_arrange(i)) P.push_back(i);
		pack_edge(puzzle, P, P2f(X1, Ym-0.5f*h), P2f(0.0f, Y1-Y0), h, w);

		P.clear(); for (int i = (H-1)*W+1; i < N-1; ++i) if (puzzle.should_arrange(i)) P.push_back(i);
		pack_edge(puzzle, P, P2f(Xm-0.5f*w, Y1), P2f(X0-X1, 0.0f), w, h);

		P.clear(); for (int i = W; i < (H-1)*W; i += W) if (puzzle.should_arrange(i)) P.push_back(i);
		pack_edge(puzzle, P, P2f(X0-w, Ym-0.5f*h), P2f(0.0f, Y0-Y1), h, w);
	}
}
