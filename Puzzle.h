#pragma once
#include <vector>
#include <set>
#include <map>
#include <cassert>
#include <functional>
#include "Vector.h"
#include "Persistence/Serializer.h"

extern double rand01(); //  0..1
extern double rand11(); // -1..1

struct Puzzle : public Serializable
{
	Puzzle(); // tries for approximately N somewhat square pieces

	void save(Serializer &s) const;
	void load(Deserializer &s);

	void reset(double W, double H, int N); // tries for approximately N somewhat square pieces
	void shuffle(bool heap = false); // lay out in a square or dump into a heap
	bool solved() const
	{
		// if we have one group that contains all the pieces and possibly
		// some number of empty groups, we're done:
		for (const auto &G : groups)
			if (!G.empty()) return G.size() == N;
		return N <= 1;
	}

	typedef int     Piece;
	typedef int     Group;
	typedef uint8_t Border;
	int W, H, N; // number of pieces in x and y directions, N = W*H is total
	std::vector<P2f>  pos; // positions of the pieces (top-left corner, size is (1,1))
	std::vector<Piece>  z; // draw in order z[0], z[1], ...
	std::vector<Group>  g; // which group is the piece in? -1 for none
	std::vector<std::set<Piece>> groups; // luxury item, simplifies the algorithms
	std::vector<bool> eh, ev; // horizontal and vertical edges, true = knob points away from zero
	std::vector<Border> borders; // for easy transfer to shaders, computed from eh+ev
	float sx, sy; // scaling factors for the pieces (internally this class uses w=h=1!)
	
	struct Anim { P2f dest, v; Anim() : dest(0.0f, 0.0f), v(0.0f, 0.0f) {} };
	std::map<Piece, Anim> animations;
	void animate(double dt);

	P2f  get(Piece i) const { assert(i >= 0 && i < N); return P2f(pos[i].x*sx, pos[i].y*sy); }
	void move(Piece i, const P2f &p, bool animate);
	void move(Piece i, float x, float y, bool animate) { move(i, P2f(x/sx,y/sy), animate); }
	bool connect(Piece i, float delta_max); // returns true for new connections
	void pick_up(Piece i); // move to top of z-order (i.e. to end of z vector)

	inline bool align(Piece i, Piece j, float delta_max) const // should they get connected?
	{
		assert(i >= 0 && i < N);
		assert(j >= 0 && j < N);
		assert(i != j);
		assert(abs(i-j) == 1 || abs(i-j) == W);
		assert(delta_max > 0.0f);
		P2f d = pos[i]-pos[j] - P2f(i%W - j%W, i/W - j/W);
		return fabs(d.x) < delta_max*sx &&
		       fabs(d.y) < delta_max*sy;
		//return (pos[i]-pos[j] - P2f(i%W - j%W, i/W - j/W)).absq() < epsq;
	}

	Piece hit_test(const P2f &p, P2f &rel) const { return hit_test(p.x, p.y, rel); }
	Piece hit_test(float x, float y, P2f &rel) const // any piece at (x,y)?
	{
		for (int j = N-1; j >= 0; --j)
		{
			int i = z[j];
			float px = pos[i].x, py = pos[i].y;
			if (x > px && x < px+1.0 && y > py && y < py+1.0)
			{
				rel.set(x-px, y-py);
				return i;
			}
		}
		return -1;
	}

	bool is_edge_piece(Piece i) const
	{
		assert(i >= 0 && i < N);
		int x = i % W, y = i / W;
		return x == 0 || x == W-1 || y == 0 || y == H-1;
	}

	void bbox(float &x0, float &x1, float &y0, float &y1, bool groups_only = false) const
	{
		int i = 0;
		if (groups_only) while (i < N && g[i] < 0) ++i;
		if (i >= N) { x0 = x1 = y0 = y1 = 0.0; return; }
		x0 = x1 = pos[i].x;
		y0 = y1 = pos[i].y;
		for (++i; i < N; ++i)
		{
			if (groups_only && g[i] < 0) continue;
			const P2f &p = pos[i];
			if (p.x < x0) x0 = p.x;
			if (p.x > x1) x1 = p.x;
			if (p.y < y0) y0 = p.y;
			if (p.y > y1) y1 = p.y;
		}
		x1 += 1.0f;
		y1 += 1.0f;
		x0 *= sx; x1 *= sx;
		y0 *= sy; y1 *= sy;
	}

	void sanity_checks() const
	{
		#if 0 //ndef NDEBUG
		assert(W >= 0 && H >= 0);
		assert(N == W*H);
		assert(pos.size() == N);
		assert(z.size() == N);
		assert(ev.size() == H*(W-1));
		assert(eh.size() == W*(H-1));
		assert(groups.size() <= N/2);
		assert(borders.size() == N);

		for (Piece p : z) assert(p >= 0 && p < N);
		assert(std::set(z.begin(), z.end()).size() == N);
		for (int i = 0; i < (int)groups.size(); ++i)
			for (Piece p : groups[i]) assert(p >= 0 && p < N && g[p] == i);
		for (int p = 0; p < N; ++p)
			assert(g[p] < 0 || groups[g[p]].count(p));
		#endif
	}
};
