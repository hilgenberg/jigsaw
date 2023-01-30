#pragma once
#include "Coordinates.h"
#include "Utility/Vector.h"
#include "Persistence/Serializer.h"
#include "Utility/Preferences.h"
#include <vector>
#include <set>
#include <map>
#include <cassert>
#include <functional>

extern double rand01(); //  0..1
extern double rand11(); // -1..1

struct Puzzle : public Serializable
{
	Puzzle() : N(0), W(0), H(0) {}
	virtual ~Puzzle() {}

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
	int    W = 0, H = 0, N = 0; // number of pieces in x and y directions, N = W*H is total
	double sx = 1.0, sy = 1.0;  // scaling factors for the pieces (internally this class uses w=h=1!)
	std::vector<PuzzleCoords>    pos;     // positions of the pieces (top-left corner, size is (1,1))
	std::vector<Piece>           z;       // draw in order z[0], z[1], ... (=> z[0] is bottom-most piece, z[N-1] top one)
	std::vector<Group>           g;       // which group is the piece in? -1 for none
	std::vector<std::set<Piece>> groups;  // luxury item, simplifies the algorithms
	std::vector<bool>            eh, ev;  // horizontal and vertical edges, true = knob points away from zero
	std::vector<Border>          borders; // for easy transfer to shaders, computed from eh and ev

	struct Anim { PuzzleCoords dest, v; Anim() : dest(0.0, 0.0), v(0.0, 0.0) {} };
	std::map<Piece, Anim> animations;
	void animate(double dt);
	void kill_animations() { animations.clear(); }
	
	inline CameraCoords   to_camera(const PuzzleCoords &p) const { return CameraCoords(p.x*sx, p.y*sy); }
	inline PuzzleCoords from_camera(const CameraCoords &p) const { return PuzzleCoords(p.x/sx, p.y/sy); }
	
	void  move(Piece i, const PuzzleCoords &p, bool animate);
	bool  drop(Piece i, double delta_max); // returns true for new connections
	void  pick_up(Piece i); // move to top of z-order (i.e. to end of z vector)
	void  magnetize(std::set<Piece> &I, PuzzleCoords dp); // recursively puts all touching pieces into magnet
	bool  drop(std::set<Piece> &I, double delta_max); // returns true for new connections
	Piece hit_test(const PuzzleCoords &p, const PuzzleCoords &radius, PuzzleCoords &rel) const; // skips all pieces with is_fixed(i)!
	bool  hit_test(Piece i, const PuzzleCoords &p) const; // helper function

	inline bool align(Piece i, Piece j, double delta_max) const // should they get connected?
	{
		assert(i >= 0 && i < N);
		assert(j >= 0 && j < N);
		assert(i != j);
		assert(abs(i-j) == 1 || abs(i-j) == W);
		assert(delta_max > 0.0);
		PuzzleCoords d = pos[i]-pos[j] - P2d(i%W - j%W, i/W - j/W);
		return fabs(d.x) < delta_max*sx &&
		       fabs(d.y) < delta_max*sy;
		//return (pos[i]-pos[j] - P2d(i%W - j%W, i/W - j/W)).absq() < epsq;
	}
	bool overlap(Piece i, Piece j) const;

	inline PuzzleCoords delta(Piece i) const // distance from final place
	{
		assert(i >= 0 && i < N);
		return pos[i] - P2d(i%W - 0.5*W, i/W - 0.5*H);
	}

	inline bool align(Piece i, double delta_max) const // should it snap into place?
	{
		assert(i >= 0 && i < N);
		assert(delta_max > 0.0);
		PuzzleCoords d = delta(i);
		return fabs(d.x) < delta_max*sx &&
		       fabs(d.y) < delta_max*sy;
	}

	inline bool is_fixed(Piece i) const
	{
		assert(i >= 0 && i < N);
		return Preferences::absolute_mode() && delta(i).absq() < 1e-12;
	}

	inline bool is_snapped(Piece i) const
	{
		assert(i >= 0 && i < N);
		return (Preferences::absolute_mode() || is_corner_group(i)) && delta(i).absq() < 1e-12;
	}

	inline bool should_arrange(Piece i) const
	{
		assert(i >= 0 && i < N);
		return g[i] < 0 && !is_fixed(i);
	}

	static void overhang(EdgeType et, float &d1, float &d0)
	{
		// returns how much bigger a piece can get when the knob sticks
		// out (d1) or when the knob goes in (d0)
		switch (et)
		{
			case None:     d1 = d0 = 0.0f; break;
			case Regular:  d1 = 0.15f; d0 = 0.08578643762690485f; break;
			case Triangle: d1 = 0.15f; d0 = 0.05f; break;
			case Groove:   d1 = d0 = 0.034f; break;
			case Circle:   d1 = 0.2f; d0 = 0.0f; break;
			default: assert(false); d1 = d0 = 0.0f; break;
		}
	}

	bool is_edge_piece(Piece i) const
	{
		assert(i >= 0 && i < N);
		//int x = i % W, y = i / W;
		//return x == 0 || x == W-1 || y == 0 || y == H-1;
		return (i % W % (W-1)) * (i / W % (H-1)) == 0;
	}
	bool is_corner(Piece i) const
	{
		assert(i >= 0 && i < N);
		//return i == 0 || i == W-1 || i == N-W || i == N-1;
		return i % W % (W-1) + i / W % (H-1) == 0;
	}
	bool is_corner_group(Piece i) const
	{
		assert(i >= 0 && i < N);
		if (is_corner(i)) return true;
		if (g[i] < 0) return false;
		return g[i] == g[0] || g[i] == g[W-1] || g[i] == g[N-W] || g[i] == g[N-1];
	}

	void bbox(double &x0, double &x1, double &y0, double &y1, bool groups_only = false) const
	// ignores prior values of xi/yi and returns CameraCoords!
	{
		int i = 0;
		if (groups_only) while (i < N && g[i] < 0) ++i;
		if (i >= N) { x0 = x1 = y0 = y1 = 0.0; return; }
		x0 = x1 = pos[i].x;
		y0 = y1 = pos[i].y;
		for (++i; i < N; ++i)
		{
			if (groups_only && g[i] < 0) continue;
			const PuzzleCoords &p = pos[i];
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

	void zsort(); // make sure single pieces don't get lost under big groups

	void sanity_checks() const
	{
		assert(W >= 0 && H >= 0);
		assert(           W*H == N);
		assert(    pos.size() == N);
		assert(      z.size() == N);
		assert(     ev.size() == H*(W-1));
		assert(     eh.size() == W*(H-1));
		assert( groups.size() <= N/2);
		assert(borders.size() == N);

		#if 0 //ndef NDEBUG
		for (Piece p : z) assert(p >= 0 && p < N);
		assert(std::set(z.begin(), z.end()).size() == N);
		for (int i = 0; i < (int)groups.size(); ++i)
			for (Piece p : groups[i]) assert(p >= 0 && p < N && g[p] == i);
		for (int p = 0; p < N; ++p)
			assert(g[p] < 0 || groups[g[p]].count(p));
		#endif
	}
};
