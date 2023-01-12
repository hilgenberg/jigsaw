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

void Puzzle::save(Serializer &s) const
{
	s.uint32_(W); s.uint32_(H);
	s.double_(sx); s.double_(sy);
	for (auto v : ev) s.bool_(v);
	for (auto v : eh) s.bool_(v);
	for (int i = 0; i < N; ++i)
	{
		s.double_(pos[i].x);
		s.double_(pos[i].y);
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
	s.double_(sx);
	s.double_(sy);

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
		s.double_(pos[i].x);
		s.double_(pos[i].y);
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
		const double spcx = 0.3*sx + 0.5, spcy = 0.3*sy + 0.5;
		for (int j = 0; j < H; ++j)
		{
			for (int i = 0; i < W; ++i)
			{
				int k = j*W + i;
				pos[z[k]].set(
					(i-0.5*W)*(sx+spcx)+0.5*spcx,
					(j-0.5*H)*(sy+spcy)+0.5*spcy);
			}
		}
	}
	else
	{
		for (int i = 0; i < N; ++i)
		{
			double x = rand01() * (W-1), y = rand01() * (H-1);
			pos[i].set(x-0.5*W, y-0.5*H);
		}
	}

	sanity_checks();
}

void Puzzle::pick_up(Piece i)
{
	assert(i >= 0 && i < N);

	if (g[i] < 0)
	{
		auto it = std::remove(z.begin(), z.end(), i);
		assert(it == z.begin()+N-1);
		z[N-1] = i;
	}
	else
	{
		auto &G = groups[g[i]];
		auto it = std::remove_if(z.begin(), z.end(), [&G](Piece p){ return G.count(p) > 0; });
		assert(it == z.begin()+N-G.size());
		int i = N-G.size();
		for (Piece j : G) z[i++] = j;
	}

	sanity_checks();
}

void Puzzle::move(Piece i, const PuzzleCoords &p, bool animate)
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
			pos[j] = pos[i] - P2d(i%W - j%W, i/W - j/W);
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
			double dq = (pos[i]-a.dest).absq();
			if (dq < std::max(0.01, a.v.absq()*dt))
			{
				it = animations.erase(it);
				pos[i] = a.dest;
				break;
			}
			else if (k == 0)
				++it;
			
			P2d dx = a.dest - pos[i]; 
			a.v += dx*dt;

			// dampen lateral movement, so we don't get orbits
			dx.to_unit();
			a.v -= (a.v - dx*(a.v*dx)) * 0.1;
		}

		if (g[i] >= 0) for (Piece j : groups[g[i]])
			pos[j] = pos[i] - P2d(i%W - j%W, i/W - j/W);
	}
}

bool Puzzle::drop(Piece i, double delta_max)
{
	assert(i >= 0 && i < N);
	assert(!animations.count(i));

	bool snap = ((Preferences::absolute_mode() || is_corner_group(i)) && delta(i).absq() > 1e-12 && align(i, delta_max));

	if (snap) move(i, P2d(i%W - 0.5*W,i/W - 0.5*H), false);

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

	if (adding.empty())
	{
		zsort();
		return snap;
	}

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
			pos[j] = pos[i] - P2d(i%W - j%W, i/W - j/W);
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

	zsort();

	sanity_checks();

	return true;
}

bool Puzzle::drop(std::set<Piece> &I, double delta_max)
{
	bool ret = false;
	while (!I.empty())
	{
		auto i = *I.begin();
		ret |= drop(i, delta_max);
		if (g[i] < 0) I.erase(i); else for (Piece j : groups[g[i]]) I.erase(j);
	}
	return false;
}

void Puzzle::zsort()
{
	std::stable_sort(z.begin(), z.end(), [this](Piece a, Piece b)
	{
		// return true if a strictly lower than b:

		// snapped pieces go below all others
		bool sa = is_snapped(a), sb = is_snapped(b);
		if (sa != sb) return sa;
		if (sa) return false;
		assert(!sa && !sb);

		// single pieces go above all groups
		if (g[b] < 0) return g[a] >= 0;
		if (g[a] < 0) return false;
		assert(g[a] >= 0 && g[b] >= 0);

		// bigger groups go below smaller groups
		size_t na = groups[g[a]].size(), nb = groups[g[b]].size();
		return na > nb;
	});
}

void Puzzle::magnetize(std::set<Piece> &I0, PuzzleCoords dp)
{
	// move the existing pieces
	std::set<Piece> I(I0);
	while (!I.empty())
	{
		auto i = *I.begin();
		move(i, pos[i]+dp, false);
		if (g[i] < 0) I.erase(i); else for (Piece j : groups[g[i]]) I.erase(j);
	}

	// check for new contacts
	I = I0; // I are the pieces left to check
	while (!I.empty())
	{
		auto i = *I.begin(); I.erase(i);
		for (int j = 0; j < N; ++j)
		{
			if (I0.count(j)) continue;
			if (is_snapped(j)) continue;
			if (!overlap(i,j)) continue;
			if (g[j] < 0) { I0.insert(j); I.insert(j); pick_up(j); } else
			for (Piece k : groups[g[j]]) { I0.insert(k); I.insert(k); pick_up(k); }
		}
	}
}

bool Puzzle::overlap(Piece i, Piece j) const
{
	assert(i >= 0 && i < N);
	assert(j >= 0 && j < N);
	//if (i == j) return true;

	auto &a = pos[i], &b = pos[j];
	if (a.x > b.x + 1.0 || b.x > a.x + 1.0) return false;
	if (a.y > b.y + 1.0 || b.y > a.y + 1.0) return false;
	return true;
}
