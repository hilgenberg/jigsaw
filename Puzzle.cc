#include "Puzzle.h"
#include <random>

Puzzle::Puzzle()
: N(0), W(0), H(0)
{
}

void Puzzle::save(Serializer &s) const
{
	s.uint32_(W);
	s.uint32_(H);
}
void Puzzle::load(Deserializer &s)
{
	uint32_t u;
	s.uint32_(u); W = u;
	s.uint32_(u); H = u;
	N = W*H;
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

	auto rand_bool = std::bind(std::uniform_int_distribution<>(0, 1), std::default_random_engine());
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

void Puzzle::shuffle(bool heap)
{
	// shuffle z-order
	auto e = std::default_random_engine();
	for (int i = 0; i < N-1; ++i)
	{
		int j = std::uniform_int_distribution<>(i, N-1)(e);
		std::swap(z[i], z[j]);
	}
	
	groups.clear();
	g.assign(N, -1);

	if (!heap)
	{
		int n = (int)std::ceil(std::sqrt(N));
		for (int i = 0; i < N; ++i)
			pos[i].set((z[i]%n)*1.5f, (z[i]/n)*1.5f);
	}
	else
	{
		auto rand_r = std::bind(std::uniform_real_distribution<>(-1.0f, 1.0f), e);
		double n = std::sqrt(N);
		for (int i = 0; i < N; ++i)
		{
			float x, y, rq;
			do{ x = rand_r(); y = rand_r(); rq = x*x+y*y; } while(rq > 1.0f);
			x *= rq; y *= rq;
			x *= rq; y *= rq;
			pos[i].set(n*x, n*y);
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

void Puzzle::move(Piece i, const P2f &p)
{
	assert(i >= 0 && i < N);
	pos[i] = p;
	// move i's group along with it
	if (g[i] >= 0) for (Piece j : groups[g[i]])
		pos[j] = pos[i] - P2f(i%W - j%W, i/W - j/W);
}

bool Puzzle::connect(Piece i, float delta_max)
{
	assert(i >= 0 && i < N);

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

	if (adding.empty()) return false;

	while (!adding.empty())
	{
		adding.insert(i);

		// find the group with the lowest index
		int g0 = -1; for (Piece a : adding) if (g[a] >= 0 && g[a] < g0) g0 = g[a];
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

		// find the biggest group (which will get to keep its position)
		int n0 = (g[i] < 0 ? 1 : groups[g[i]].size());
		for (Piece a : adding) if (g[a] >= 0 && groups[g[a]].size() > n0) { i = a; n0 = groups[g[a]].size(); }

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

			// and recursion
			int x = j % W, y = j / W;
			if (x > 0   && g0 != g[j-1] && align(j, j-1, delta_max)) adding.insert(j-1);
			if (x < W-1 && g0 != g[j+1] && align(j, j+1, delta_max)) adding.insert(j+1);
			if (y > 0   && g0 != g[j-W] && align(j, j-W, delta_max)) adding.insert(j-W);
			if (y < H-1 && g0 != g[j+W] && align(j, j+W, delta_max)) adding.insert(j+W);
		}
	}
	while (groups.back().empty()) groups.pop_back();

	pick_up(i);

	sanity_checks();

	return true;
}

