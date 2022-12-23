#include "Puzzle.h"
#include <random>

static auto engine = std::default_random_engine(std::chrono::system_clock::now().time_since_epoch().count());
double rand01()
{
	static auto r = std::bind(std::uniform_real_distribution<>(0.0, 1.0), ::engine);
	return r();
}
double rand11()
{
	static auto r = std::bind(std::uniform_real_distribution<>(-1.0, 1.0), ::engine);
	return r();
}

Puzzle::Puzzle()
: N(0), W(0), H(0)
{
}

void Puzzle::save(Serializer &s) const
{
	s.uint32_(W); s.uint32_(H);
	s.float_(sx); s.float_(sy);
	for (auto v : ev) s.bool_(v);
	for (auto v : eh) s.bool_(v);
	for (int i = 0; i < N; ++i)
	{
		s.float_(pos[i].x);
		s.float_(pos[i].y);
		s.int32_(g[i]);
		s.int32_(z[i]);
	}
}
void Puzzle::load(Deserializer &s)
{
	uint32_t u;
	int32_t  k;
	s.uint32_(u); W = u;
	s.uint32_(u); H = u;
	N = W*H;
	s.float_(sx);
	s.float_(sy);

	g.resize(N);
	pos.resize(N);
	borders.resize(N);
	z.resize(N);

	bool b;
	ev.resize(H*(W-1)); for (auto v : ev) { s.bool_(b); v = b; }
	eh.resize(W*(H-1)); for (auto v : eh) { s.bool_(b); v = b; }
	int g_max = -1;
	for (int i = 0; i < N; ++i)
	{
		s.float_(pos[i].x);
		s.float_(pos[i].y);
		s.int32_(k); g[i] = k; if (k > g_max) g_max = k;
		s.int32_(k); z[i] = k;

		int x = i % W, y = i / W;
		int l = (x ==  0  ? 0 : 1+ev[y*(W-1) + x-1]);
		int r = (x == W-1 ? 0 : 2-ev[y*(W-1) + x  ]);
		int t = (y ==  0  ? 0 : 1+eh[(y-1)*W + x  ]);
		int b = (y == H-1 ? 0 : 2-eh[y*W     + x  ]);
		borders[i] = l | (r << 2) | (t << 4) | (b << 6);
	}
	groups.clear();
	groups.resize(g_max+1);
	for (int i = 0; i < N; ++i) if (g[i] >= 0) groups[g[i]].insert(i);
}

void Puzzle::reset(double W0, double H0, int N0)
{
	if (N0 < 1) N0 = 1;
	if (W0 < 1.0) W0 = 1.0;
	if (H0 < 1.0) H0 = 1.0;

	// Start with N0 = w*h, w/h = W0/H0
	// => w = W0/H0*h => N0 = W0/H0*h * h
	// => h = sqrt(N0 * H0/W0)
	//    w = sqrt(N0 * W0/H0)
	int w = std::max(1.0, std::round(std::sqrt(N0 * W0/H0)));
	int h = std::max(1.0, std::round(std::sqrt(N0 * H0/W0)));
	W = w; H = h; // will get overwritten, but keep analyzer happy

	// look around these values for best aspect ratio and closest to N0 pieces
	double q0 = -1.0; // quality rating of current best solution
	for (int a = -10; a <= 10; ++a)
	{
		if (w+a <= 0) continue;
		for (int b = -10; b <= 10; ++b)
		{
			if (h+b <= 0) continue;
			double pw = W0/(w+a), ph = H0/(h+b); 
			double r = pw/ph; if (r > 1.0) r = 1.0/r;
			double s = (double)(w+a)*(h+b)/N0; if (s > 1.0) s = 1.0/s;
			double q = r*s;
			if (q < q0) continue;
			q0 = q;
			W = w+a; H = h+b;
		}
	}
	assert(q0 >= 0.0);

	N = W*H;

	sx = W0/W;
	sy = H0/H;
	if (sx > sy) { sx /= sy; sy = 1.0f; }
	else         { sy /= sx; sx = 1.0f; }

	groups.clear();
	g.resize(N, -1);

	auto rand_bool = std::bind(std::uniform_int_distribution<>(0, 1), engine);
	ev.resize(H*(W-1)); for (auto v : ev) v = rand_bool();
	eh.resize(W*(H-1)); for (auto v : eh) v = rand_bool();

	pos.resize(N);
	borders.resize(N);
	z.resize(N);
	for (int i = 0; i < N; ++i)
	{
		int x = i % W, y = i / W;
		pos[i].set(x, y);
		z[i] = i;

		int l = (x ==  0  ? 0 : 1+ev[y*(W-1) + x-1]);
		int r = (x == W-1 ? 0 : 2-ev[y*(W-1) + x  ]);
		int t = (y ==  0  ? 0 : 1+eh[(y-1)*W + x  ]);
		int b = (y == H-1 ? 0 : 2-eh[y*W     + x  ]);
		borders[i] = l | (r << 2) | (t << 4) | (b << 6);
	}
}

void Puzzle::shuffle(bool grid)
{
	// shuffle z-order
	for (int i = 0; i < N-1; ++i)
	{
		int j = std::uniform_int_distribution<>(i, N-1)(engine);
		std::swap(z[i], z[j]);
	}
	
	groups.clear();
	g.assign(N, -1);

	if (grid)
	{
		const float spcx = 0.3*sx + 0.5f, spcy = 0.3*sy + 0.5f;
		for (int j = 0; j < H; ++j)
		{
			for (int i = 0; i < W; ++i)
			{
				int k = j*W + i;
				pos[z[k]].set(
					(i-0.5f*W)*(sx+spcx)+0.5f*spcx,
					(j-0.5f*H)*(sy+spcy)+0.5f*spcy);
			}
		}
	}
	else
	{
		for (int i = 0; i < N; ++i)
		{
			float x = rand01() * (W-1), y = rand01() * (H-1);
			pos[i].set(x-0.5f*W, y-0.5f*H);
		}
	}

	sanity_checks();
}

void Puzzle::pick_up(Piece i)
{
	assert(i >= 0 && i < N);

	bool is_final = (Preferences::absolute_mode() || is_big_border_group(i)) && delta(i).absq() < 1e-12;

	if (g[i] < 0)
	{
		if (is_final && z[0] == i) return;
		auto it = std::remove(z.begin(), z.end(), i);
		assert(it == z.begin()+N-1);
		if (!is_final)
		{
			z[N-1] = i;
		}
		else
		{
			for (int j = N-1; j > 0; --j) z[j] = z[j-1];
			z[0] = i;
		}
	}
	else
	{
		auto &G = groups[g[i]];
		if (is_final)
		{
			bool all_good = true; // is the entire group at bottom already?
			for (int i = 0; i < (int)G.size(); ++i)
			{
				if (G.count(z[i])) continue;
				all_good = false;
				break;
			}
			if (all_good) return;
		}

		auto it = std::remove_if(z.begin(), z.end(), [&G](Piece p){ return G.count(p) > 0; });
		assert(it == z.begin()+N-G.size());
		int i = N-G.size();
		if (is_final)
		{
			while (--i >= 0) z[i+G.size()] = z[i];
			i = 0;
		}
		for (Piece j : G) z[i++] = j;

	}

	sanity_checks();
}

void Puzzle::move(Piece i, const P2f &p, bool animate)
{
	assert(i >= 0 && i < N);
	if (!animate)
	{
		animations.erase(i);
		pos[i] = p;
		// move i's group along with it
		if (g[i] >= 0) for (Piece j : groups[g[i]])
		{
			animations.erase(j);
			pos[j] = pos[i] - P2f(i%W - j%W, i/W - j/W);
		}
	}
	else
	{
		if (g[i] >= 0) for (Piece j : groups[g[i]]) if (j != i) animations.erase(j);
		auto &a = animations[i];
		a.dest = p; // but leave v as it is, if a already existed
	}
}

void Puzzle::animate(double dt)
{
	//std::cout << "anim " << animations.size() << ", "  << dt << std::endl;
	for (auto it = animations.begin(); it != animations.end();)
	{
		Piece i = it->first;
		Anim &a = it->second;
		for (int k = 5; k >= 0; --k)
		{
			pos[i] += a.v * dt;
			float dq = (pos[i]-a.dest).absq();
			if (dq < std::max(0.01, a.v.absq()*dt))
			{
				it = animations.erase(it);
				pos[i] = a.dest;
				break;
			}
			else if (k == 0)
				++it;
			
			P2f dx = a.dest - pos[i]; 
			a.v += dx*dt;

			// dampen lateral movement, so we don't get orbits
			dx.to_unit();
			a.v -= (a.v - dx*(a.v*dx)) * 0.1;
		}

		if (g[i] >= 0) for (Piece j : groups[g[i]])
			pos[j] = pos[i] - P2f(i%W - j%W, i/W - j/W);
	}
}

bool Puzzle::connect(Piece i, float delta_max)
{
	assert(i >= 0 && i < N);
	assert(!animations.count(i));

	bool snap = (Preferences::absolute_mode() && delta(i).absq() > 1e-12 && align(i, delta_max));

	if (snap) move(i, P2f(i%W - 0.5f*W,i/W - 0.5f*H), false);

	// check for new connections
	std::set<Piece> adding;
	if (g[i] < 0)
	{
		int x = i % W, y = i / W;
		if (x > 0   && align(i, i-1, delta_max)) adding.insert(i-1);
		if (x < W-1 && align(i, i+1, delta_max)) adding.insert(i+1);
		if (y > 0   && align(i, i-W, delta_max)) adding.insert(i-W);
		if (y < H-1 && align(i, i+W, delta_max)) adding.insert(i+W);
	}
	else for (Piece j : groups[g[i]])
	{
		int x = j % W, y = j / W;
		if (x > 0   && g[i] != g[j-1] && align(j, j-1, delta_max)) adding.insert(j-1);
		if (x < W-1 && g[i] != g[j+1] && align(j, j+1, delta_max)) adding.insert(j+1);
		if (y > 0   && g[i] != g[j-W] && align(j, j-W, delta_max)) adding.insert(j-W);
		if (y < H-1 && g[i] != g[j+W] && align(j, j+W, delta_max)) adding.insert(j+W);
	}

	if (adding.empty()) return snap;

	do {
		adding.insert(i);

		// find the group with the lowest index
		int g0 = -1; for (Piece a : adding) if (g[a] >= 0 && (g0 < 0 || g[a] < g0)) g0 = g[a];
		if (g0 < 0)
		{
			// create new group (or recycle an empty one)
			int K = (int)groups.size();
			for (int k = 0; k < K; ++k) if (groups[k].empty())
			{
				g0 = k;
				break;
			}
			if (g0 < 0)
			{
				g0 = K;
				groups.emplace_back();
			}
		}
		assert(g0 >= 0);
		auto &G = groups[g0];

		if (!snap)
		{
			// find the biggest group (which will get to keep its position)
			int n0 = (g[i] < 0 ? 1 : groups[g[i]].size());
			for (Piece a : adding) if (g[a] >= 0 && groups[g[a]].size() > n0) { i = a; n0 = groups[g[a]].size(); }
		}

		// merge the full groups
		for (Piece a : adding)
		{
			if (g[a] < 0) { g[a] = g0; G.insert(a); continue; }
			if (g[a] != g0)
			{
				auto &H = groups[g[a]];
				for (Piece j : H) g[j] = g0;
				G.insert(H.begin(), H.end());
				H.clear();
			}
		}
		adding.clear();

		for (Piece j : G)
		{
			// put into exact location
			pos[j] = pos[i] - P2f(i%W - j%W, i/W - j/W);
			animations.erase(j);

			// and recursion
			int x = j % W, y = j / W;
			if (x > 0   && g0 != g[j-1] && align(j, j-1, delta_max)) adding.insert(j-1);
			if (x < W-1 && g0 != g[j+1] && align(j, j+1, delta_max)) adding.insert(j+1);
			if (y > 0   && g0 != g[j-W] && align(j, j-W, delta_max)) adding.insert(j-W);
			if (y < H-1 && g0 != g[j+W] && align(j, j+W, delta_max)) adding.insert(j+W);
		}
	} while (!adding.empty());

	while (groups.back().empty()) groups.pop_back();

	pick_up(i);

	sanity_checks();

	return true;
}

bool Puzzle::is_big_border_group(Piece i) const
{
	// big border group = group containing more than half the edge pieces
	if (i < 0 || i >= N) return false;
	if (g[i] < 0 || groups[g[i]].size() <= W+H-1) return false; // wrong for W or H == 1, but that's ok
	// total size is not enough - we need to count the number of border pieces:
	auto &G = groups[g[i]]; int n = 0;
	for (int x = 0; x < W; ++x)
	{
		if (G.count(x)) ++n;
		if (G.count(W*(H-1)+x)) ++n;
	}
	for (int y = 1; y < H-1; ++y)
	{
		if (G.count(W*y)) ++n;
		if (G.count(W*y+W-1)) ++n;
	}
	return n > W+H-1;
}

// helper functions from the shader code:
static inline int tx(int b) { return (b & 1) - ((b>>1) & 1); }
static inline float dq(const P2f &v, float r) { return v.absq() - r*r; }
static inline float min(float a, float b) { return std::min(a,b); }
static inline float max(float a, float b) { return std::max(a,b); }
Puzzle::Piece Puzzle::hit_test(float x, float y, P2f &rel) const // any piece at (x,y)?
{
	float d1, d0;
	const auto edge = Preferences::edge();
	overhang(edge, d1, d0);
	float d = std::max(d1, d0);

	for (int j = N-1; j >= 0; --j)
	{
		const int i = z[j];
		const float px = pos[i].x, py = pos[i].y;
		if (x < px-d || x > px+1.0f+d || y < py-d || y > py+1.0f+d) continue;
		
		const int b = (int)borders[i];
		const int bl = tx(b);
		const int br = tx(b>>2);
		const int bt = tx(b>>4);
		const int bb = tx(b>>6);

		float x0 = 0.0f, x1 = 1.0f, y0 = 0.0f, y1 = 1.0f;
		if (bl == 1) x0 -= d1; else if (bl == -1) x0 -= d0;
		if (br == 1) x1 += d1; else if (br == -1) x1 += d0;
		if (bt == 1) y0 -= d1; else if (bt == -1) y0 -= d0;
		if (bb == 1) y1 += d1; else if (bb == -1) y1 += d0;
		if (x-px < x0 || x-px > x1 || y-py < y0 || y-py > y1) continue;

		bool hit = true;
		P2f orig(x-px, y-py);
		#define discard hit=false
		#define vec2 P2f
		switch (edge)
		{
			case None: break;
			case Regular:
			{
				const float R = 1.5, r = 0.15, d0 = 0.0, h = 0.08578643762690485;
				if (bl*max(dq(vec2(    bl*(h-R), 0.5)-orig, R), -dq(vec2(   -bl*d0, 0.5)-orig, r)) < -1e-8) discard;
				if (br*max(dq(vec2(1.0+br*(R-h), 0.5)-orig, R), -dq(vec2(1.0+br*d0, 0.5)-orig, r)) < -1e-8) discard;
				if (bt*max(dq(vec2(0.5,     bt*(h-R))-orig, R), -dq(vec2(0.5,    -bt*d0)-orig, r)) < -1e-8) discard;
				if (bb*max(dq(vec2(0.5, 1.0+bb*(R-h))-orig, R), -dq(vec2(0.5, 1.0+bb*d0)-orig, r)) < -1e-8) discard;
				break;
			}
			case Triangle:
			{
				const float h1 = 0.15f, h2 = 0.05f, w = 0.5f/3.0f;
				const float x = fabs(orig.x-0.5), y = fabs(orig.y-0.5f);
				const float a = (h1+h2)/w, b = h1*h2/(h1-h2);
				const float xx = min(a*x - h1, b - 2.0f*b*x);
				const float yy = min(a*y - h1, b - 2.0f*b*y);
				if (     orig.y < bt * xx) discard;
				if (1.0f-orig.y < bb * xx) discard;
				if (     orig.x < bl * yy) discard;
				if (1.0f-orig.x < br * yy) discard;
				break;
			}
			case Groove:
			{
				float h = 0.034, r = 0.2;
				if (bt*fabs(orig.x-0.5) > bt*r && orig.y <     h) discard;
				if (bb*fabs(orig.x-0.5) > bb*r && orig.y > 1.0-h) discard;
				if (bl*fabs(orig.y-0.5) > bl*r && orig.x <     h) discard;
				if (br*fabs(orig.y-0.5) > br*r && orig.x > 1.0-h) discard;
				break;
			}
			case Circle:
			{
				const float r = 0.2f;
				const float x = fabs(orig.x-0.5f), y = fabs(orig.y-0.5f);
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
	return -1;
}
