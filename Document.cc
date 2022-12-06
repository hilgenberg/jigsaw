#include "Document.h"
#include "../Persistence/Serializer.h"
#include "shaders.h"
#include "OpenGL/GL_Util.h"
#include <map>
#include <vector>
#include <cassert>
#include <GL/gl.h>
#include <GL/glu.h>
#include <iostream>
#include <sstream>
#include "Histogram.h"

//------------------------------------------------------------------
// construction and drawing
//------------------------------------------------------------------

static GLuint program = 0;
static GLuint bg_program = 0;
static GLuint VBO[2] = {0,0}, VAO[2] = {0,0}; // use double buffering for data to avoid stalls in glMap
static int current_buf = 0;
static GLuint texture = 0; 

Document::Document()
: bg(0.25f)
{
}
void Document::load(const std::string &p, int N)
{
	if (N <= 0) N = 1000;
	free_all();
	im.load(im_path = p); histo = nullptr;
	puzzle.reset(im.w(), im.h(), N);
	puzzle.shuffle(false);
	reset_view();
	init();
}
void Document::init()
{
	if (!program)
	{
		std::map<GLuint, const char *> shaders = {
			{GL_VERTEX_SHADER,   vertex},
			{GL_GEOMETRY_SHADER, geometry},
			{GL_FRAGMENT_SHADER, fragment}
		};
		program = compileShaders(shaders);
		GL_CHECK;
	}
	if (!bg_program)
	{
		std::map<GLuint, const char *> shaders = {
			{GL_VERTEX_SHADER,   bg_vertex},
			{GL_FRAGMENT_SHADER, bg_fragment}
		};
		bg_program = compileShaders(shaders);
		GL_CHECK;
	}

	int N = puzzle.N;
	assert(!texture);
	glGenTextures(1, &texture);
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, texture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, im.w(), im.h(), 0, GL_RGBA, GL_UNSIGNED_BYTE, im.data().data());
	glGenerateMipmap(GL_TEXTURE_2D);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
	float borderColor[] = { 0.0f, 0.0f, 0.0f, 1.0f };
	glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);  
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	GL_CHECK;

	glGenVertexArrays(3, VAO);
	glGenBuffers(2, VBO);
	for (int i = 0; i < 2; ++i)
	{
		assert(sizeof(Puzzle::Border) == 1);
		glBindVertexArray(VAO[i]);
		glBindBuffer(GL_ARRAY_BUFFER, VBO[i]);
		glBufferData(GL_ARRAY_BUFFER, N*(5*sizeof(float) + 1), NULL, GL_DYNAMIC_DRAW);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5*sizeof(float), (void*)0); // pos
		glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5*sizeof(float), (void*)(3*sizeof(float))); // tex
		glVertexAttribIPointer(2, 1, GL_UNSIGNED_BYTE,  0, (void*)(5*sizeof(float)*N)); // border
		glEnableVertexAttribArray(0);
		glEnableVertexAttribArray(1);
		glEnableVertexAttribArray(2);
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, texture);
		glBindBuffer(GL_ARRAY_BUFFER, 0); 
		GL_CHECK;
	}

	glBindVertexArray(0); 
	GL_CHECK;
}
void Document::free_all()
{
	glDeleteVertexArrays(3, VAO); VAO[0] = VAO[1] = VAO[2] = 0;
	glDeleteBuffers(2, VBO); VBO[0] = VBO[1] = 0;
	glDeleteTextures(1, &texture); texture = 0;
}

void Document::load(Deserializer &s)
{
	free_all();
	s.string_(im_path); std::cout << im_path << std::endl;
	s.member_(puzzle);
	s.member_(camera);
	s.marker_("EOF.");
	im.load(im_path); histo = nullptr;
	init();
}
void Document::save(Serializer &s) const
{
	s.string_(im_path);
	s.member_(puzzle);
	s.member_(camera);
	s.marker_("EOF.");
}

void Document::draw()
{
	GL_CHECK;
	const int N = puzzle.N;
	if (camera.empty()) return;

	// clear background
	bg.set_clear();
	glClearDepth(1.0);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	GL_CHECK;

	// draw assembly area
	glDepthMask(GL_FALSE);
	glDisable(GL_DEPTH_TEST);
	glUseProgram(bg_program);
	glBindVertexArray(VAO[2]); // empty but needed
	camera.set(0);
	glUniform2f(1, 0.5f*puzzle.W, 0.5f*puzzle.H);
	//glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
	GL_CHECK;
	glBindVertexArray(0);

	// send current data
	if (N <= 0) return;
	GL_CHECK;
	const int W = puzzle.W, H = puzzle.H;
	float sx = puzzle.sx, sy = puzzle.sy;
	float *data = (float*)glMapNamedBuffer(VBO[current_buf], GL_WRITE_ONLY);
	unsigned char *d = (unsigned char*)(data+5*N);
	for (int i = 0; i < N; ++i)
	{
		const P2f &p = puzzle.pos[i];
		data[5*i] = p.x*sx; data[5*i+1] = p.y*sy;
		data[5*puzzle.z[i]+2] = 1.0 - 2.0*(i+1)/(N+1); // with 24-bit depth buffer should work up to 16.7 million pieces
	 	float x = i%W, y = i/W;
		data[5*i+3] = x/W; data[5*i+4] = y/H;
		d[i] = (int)puzzle.borders[i];
	}
	GL_CHECK;
	glUnmapNamedBuffer(VBO[current_buf]);

	// draw puzzle
	glUseProgram(program);
	glBindVertexArray(VAO[current_buf]);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, texture);
	camera.set(0);
	glUniform1i(1, 0); // wants the texture unit, not the texture ID!
	glUniform2i(2, W, H);
	glUniform2f(3, puzzle.sx, puzzle.sy);
	GL_CHECK;
	glDepthMask(GL_TRUE);
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS);
	GL_CHECK;
	glDrawArrays(GL_POINTS, 0, N);
	GL_CHECK;

	++current_buf; current_buf %= 2;
	GL_CHECK;
}

Document::~Document()
{
	free_all();
	if (program) glDeleteProgram(program); program = 0;
}

//------------------------------------------------------------------
// actions
//------------------------------------------------------------------

void Document::reset_view()
{
	float x0, x1, y0, y1;
	puzzle.bbox(x0, x1, y0, y1);
	camera.view_box(x0, x1, y0, y1, 1.5f);
}

void Document::arrange()
{
	// leave space for assembled pieces
	float X0, X1, Y0, Y1;
	puzzle.bbox(X0, X1, Y0, Y1, true);
	
	// leave space for assembly area
	const int W = puzzle.W, H = puzzle.H;
	if (W*H > 2*(W+H)+4) // only if we can go around fully
	{
		if (X0 > -0.5f*W) X0 = -0.5f*W; if (X1 < 0.5f*W) X1 = 0.5f*W;
		if (Y0 > -0.5f*H) Y0 = -0.5f*H; if (Y1 < 0.5f*H) Y1 = 0.5f*H;
	}
	if (X1 > X0+0.123 || Y1 > Y0+0.123)
	{
		X0 -= 2.0; X1 += 2.0;
		Y0 -= 2.0; Y1 += 2.0;
	}

	const float w = puzzle.sx, h = puzzle.sy;
#if 0
	const double rp = 0.5*1.3*hypot(w, h);
	P2f c(0.5f*(X0+X1)-0.5f*w, 0.5f*(Y0+Y1)-0.5f*h);
	double r = 0.5*hypot(X1-X0, Y1-Y0), a = atan2(Y0-Y1, X1-X0);
	bool ani = (puzzle.N < 1000000);
	for (int i : puzzle.z)
	{
		if (puzzle.g[i] >= 0) continue;
		float x = c.x + r*sin(a);
		float y = c.y - r*cos(a);
		puzzle.move(i, x, y, ani);
		a += 2.0*rp/r;
		r += 2.0*rp/r * rp/M_PI;
	}
#else
	const float spcx = 0.3*w + 0.5f, spcy = 0.3*h + 0.5f;
	int nx = std::ceil((X1-X0-spcx) / (w+spcx));
	int ny = std::ceil((Y1-Y0-spcy) / (h+spcy));
	X0 = 0.5f*(X0+X1) - 0.5f*(nx > 0 ? nx*(w+spcx)-spcx : 0.0f);
	Y0 = 0.5f*(Y0+Y1) - 0.5f*(ny > 0 ? ny*(h+spcy)-spcy : 0.0f);

	int x0 = 0, x1 = nx, y0 = 0, y1 = ny;
	int ix = 0, iy = -1, dx = 1, dy = 0;
	if (nx == 0) { dx = 0; dy = 1; }

	bool ani = (puzzle.N < 1000000);
	for (int i : puzzle.z)
	{
		if (puzzle.g[i] >= 0) continue;
		float x = X0 + ix * (w+spcx);
		float y = Y0 + iy * (h+spcy);
		puzzle.move(i, x, y, ani);
		ix += dx;
		iy += dy;
		if      (dx ==  1 && ix >= x1  ) { dx =  0; dy =  1; --y0; }
		else if (dy ==  1 && iy >= y1  ) { dx = -1; dy =  0; ++x1; }
		else if (dx == -1 && ix <= x0-1) { dx =  0; dy = -1; ++y1; }
		else if (dy == -1 && iy <= y0-1) { dx =  1; dy =  0; --x0; }
	}
#endif
}

static void edge_bbox(const Puzzle &puzzle, float &x0, float &x1, float &y0, float &y1)
{
	if (puzzle.W < 2 || puzzle.H < 2) { x0 = x1 = y0 = y1 = 0.0f; return; }
	int W = puzzle.W, H = puzzle.H;
	x0 = x1 = puzzle.pos[W+1].x;
	y0 = y1 = puzzle.pos[W+1].y;
	for (int y = 1; y < H-1; ++y)
	{
		for (int x = 1; x < W-1; ++x)
		{
			const P2f &p = puzzle.pos[W*y + x];
			if (p.x < x0) x0 = p.x;
			if (p.x > x1) x1 = p.x;
			if (p.y < y0) y0 = p.y;
			if (p.y > y1) y1 = p.y;
		}
	}
	x1 += 1.0f;
	y1 += 1.0f;
	x0 *= puzzle.sx; x1 *= puzzle.sx;
	y0 *= puzzle.sy; y1 *= puzzle.sy;
}

static inline bool even(int i) { return !(i&1); }

static void pack_edge(Puzzle &puzzle, std::vector<int> &P, P2f c, P2f r, float w, float h)
// c +- r/2 are the edge's endpoints
// w, h are the pieces' dimensions along and across the edge
{
	std::sort(P.begin(), P.end(), [&puzzle](int a, int b) { return puzzle.z[a] > puzzle.z[b]; });
	const float spcx = 0.3*w + 0.5f, spcy = 0.3*h + 0.5f;
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

void Document::arrange_edges()
{
	if (puzzle.W < 2 || puzzle.H < 2) { arrange(); return; }
	int W = puzzle.W, H = puzzle.H, N = puzzle.N;
	float X0, X1, Y0, Y1; edge_bbox(puzzle, X0, X1, Y0, Y1);
	if (X0 > -0.5f*W) X0 = -0.5f*W;
	if (Y0 > -0.5f*H) Y0 = -0.5f*H;
	if (X1 <  0.5f*W) X1 =  0.5f*W;
	if (Y1 <  0.5f*H) Y1 =  0.5f*H;
	X0 -= 2.0;
	Y0 -= 2.0;
	X1 += 2.0;
	Y1 += 2.0;
	float Xm = (X0+X1)*0.5f, Ym = (Y0+Y1)*0.5f;
	float w = puzzle.sx;
	float h = puzzle.sy;

	if (puzzle.g[0]   < 0) puzzle.move(  0, X0-w, Y0-h, true);
	if (puzzle.g[W-1] < 0) puzzle.move(W-1, X1,   Y0-h, true);
	if (puzzle.g[N-W] < 0) puzzle.move(N-W, X0-w, Y1,   true);
	if (puzzle.g[N-1] < 0) puzzle.move(N-1, X1,   Y1,   true);

	std::vector<int> P; for (int i = 1; i < W-1; ++i) if (puzzle.g[i] < 0) P.push_back(i);
	pack_edge(puzzle, P, P2f(Xm-0.5f*w, Y0-h), P2f(X1-X0, 0.0f), w, h);

	P.clear(); for (int i = W+W-1; i < N-1; i += W) if (puzzle.g[i] < 0) P.push_back(i);
	pack_edge(puzzle, P, P2f(X1, Ym-0.5f*h), P2f(0.0f, Y1-Y0), h, w);

	P.clear(); for (int i = (H-1)*W+1; i < N-1; ++i) if (puzzle.g[i] < 0) P.push_back(i);
	pack_edge(puzzle, P, P2f(Xm-0.5f*w, Y1), P2f(X0-X1, 0.0f), w, h);

	P.clear(); for (int i = W; i < (H-1)*W; i += W) if (puzzle.g[i] < 0) P.push_back(i);
	pack_edge(puzzle, P, P2f(X0-w, Ym-0.5f*h), P2f(0.0f, Y0-Y1), h, w);
}

void Document::move(int piece, double R)
{
	assert(piece >= 0 && piece < puzzle.N);
	P2d p = (P2d)puzzle.pos[piece];
	p.x += 0.5; p.x *= puzzle.sx;
	p.y += 0.5; p.y *= puzzle.sy;
	p -= camera.center;
	p.to_unit();
	if (!std::isfinite(p.x)) p.set(-1.0, 0.0);
	p *= R;
	p += camera.center;
	p.x -= 0.5*puzzle.sx;
	p.y -= 0.5*puzzle.sy;
	puzzle.move(piece, p.x, p.y, true);
}

static double rand_dr()
{
	double x = rand01();
	return 5.0*x*x;
}

void Document::hide(int piece, bool and_similar)
{
	if (piece < 0 || piece >= puzzle.N) return;
	double R0 = std::max(1.5*camera.range.abs(), hypot(puzzle.W, puzzle.H));

	if (and_similar)
	{
		if (!histo) histo.reset(new Histogram(im, puzzle.W, puzzle.H));
		for (int i = 0; i < puzzle.N; ++i)
		{
			if (puzzle.g[i] >= 0) continue;
			if (histo->distance(piece, i) < 0.2) move(i, R0 + rand_dr());
		}
	}
	else
	{
		move(piece, R0 + rand_dr());
	}
}

int Document::hit_test(int mx, int my, bool pick_up, P2f &rel)
{
	auto p = camera.convert(mx, my);
	p.x /= puzzle.sx;
	p.y /= puzzle.sy;
	int i = puzzle.hit_test(p, rel);
	if (pick_up && i >= 0) puzzle.pick_up(i);
	return i;
}
void Document::drag(int piece, const P2f &rel, int mx, int my, P2d &v)
{
	int w = camera.screen_w(), h = camera.screen_h();
	int pn = std::min(w, h) / 10;
	v.clear();
	if (mx <= pn) v.x = (double)(pn-mx)/pn; else if (mx >= w-1-pn) v.x = -(double)(mx-(w-1-pn))/pn;
	if (my <= pn) v.y = (double)(pn-my)/pn; else if (my >= h-1-pn) v.y = -(double)(my-(h-1-pn))/pn;

	if (piece < 0 || piece >= puzzle.N) return;
	auto p = camera.convert(mx, my);
	p.x /= puzzle.sx;
	p.y /= puzzle.sy;
	p -= rel;
	puzzle.move(piece, p, false);
}
bool Document::drop(int piece)
{
	if (piece < 0 || piece >= puzzle.N) return false;
	return puzzle.connect(piece, std::max(5.0*camera.pixel_size(), 0.3));
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
		if (puzzle.g[i] >= 0) continue;
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
