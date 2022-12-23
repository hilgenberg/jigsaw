#include "Victory.h"
#include "Puzzle.h"
#include "Camera.h"

static P2d    cam_p0;
static double cam_R0;
static P2f    puz_p0;

VictoryAnimation::VictoryAnimation(Puzzle &puzzle, Camera &camera)
: puzzle(puzzle), camera(camera), t(0.0)
{
	const int W = puzzle.W, H = puzzle.H;
	cam_p0 = camera.center;
	cam_R0 = camera.R;
	puz_p0 = puzzle.N > 0 ? puzzle.pos[0]+P2f(W*0.5f, H*0.5f) : P2f(0,0);
	puzzle.kill_animations();
}

static inline double S(double t) { assert(t >= 0.0 && t <= 1.0); return 0.5-0.5*cos(t*M_PI); }

void VictoryAnimation::run(double dt)
{
	t += dt*4.0;

	const int W = puzzle.W, H = puzzle.H;
	const int w = camera.screen_w(), h = camera.screen_h();
	double R = 1.125 * std::max(W*puzzle.sx*h, H*puzzle.sy*w) * 0.5 / std::max(w, h); // see Camera::view_box(...)
	P2f c(W*0.5f, H*0.5f);
	if (t < 1.0)
	{
		double tt = S(t);
		camera.set_R((1.0-tt)*cam_R0 + tt*R);
		camera.center = cam_p0*(1.0-tt);
		c += puz_p0*(1.0-tt);
	}
	else
	{
		camera.set_R(R);
		camera.center.clear();
	}

	for (int y = 0; y < H; ++y)
	{
		for (int x = 0; x < W; ++x)
		{
			P2f &p = puzzle.pos[W*y+x];
			p.set(x-c.x, y-c.y);
			#if 1
			if      (t < 1.0) { p.x *= 1.0+S(t); }
			else if (t < 2.0) { p.x *= 2.0; p.y *= 1.0+S(t-1.0); }
			else if (t < 3.0) { p   *= 4.0-t; }
			#else
			if      (t < 1.0) { p.x *= 1.0+t; }
			else if (t < 2.0) { p.x *= 2.0; p.y *= t; }
			else if (t < 3.0) { p   *= 4.0-t; }
			#endif
		}
	}
}

bool VictoryAnimation::done() const
{
	return t >= 3.0;
}
