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

//------------------------------------------------------------------
// construction and drawing
//------------------------------------------------------------------

static GLuint program = 0;
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
	im.load(im_path = p);
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
	//glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	GL_CHECK;

	glGenVertexArrays(2, VAO);
	glGenBuffers(2, VBO);
	for (int i = 0; i < 2; ++i)
	{
		assert(sizeof(Puzzle::Border) == 1);
		glBindVertexArray(VAO[i]);
		glBindBuffer(GL_ARRAY_BUFFER, VBO[i]);
		glBufferData(GL_ARRAY_BUFFER, N*(4*sizeof(float) + 1), NULL, GL_DYNAMIC_DRAW);
		glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, (void*)0);
		glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, (void*)(2*sizeof(float)*N));
		glVertexAttribIPointer(2, 1, GL_UNSIGNED_BYTE,  0, (void*)(4*sizeof(float)*N));
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
	glDeleteVertexArrays(2, VAO); VAO[0] = VAO[1] = 0;
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
	im.load(im_path);
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
	if (camera.empty() || N <= 0) return;

	GL_CHECK;
	float sx = puzzle.sx, sy = puzzle.sy;
	float *data = (float*)glMapNamedBuffer(VBO[current_buf], GL_WRITE_ONLY);
	for (int i : puzzle.z) { const P2f &p = puzzle.pos[i]; *data++ = p.x*sx; *data++ = p.y*sy; }
	const int W = puzzle.W, H = puzzle.H;
	for (int i : puzzle.z) { float x = i%W, y = i/W; *data++ = x/W; *data++ = y/H; }
	unsigned char *d = (unsigned char*)data;
	for (int i : puzzle.z) { *d++ = (int)puzzle.borders[i]; }
	GL_CHECK;

	glUnmapNamedBuffer(VBO[current_buf]);
	glUseProgram(program);
	glBindVertexArray(VAO[current_buf]);
	GL_CHECK;

	bg.set_clear();
	glClearDepth(1.0);
	glDepthMask(GL_TRUE);
	glDisable(GL_DEPTH_TEST);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	GL_CHECK;

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, texture);


	camera.set(0);
	glUniform1i(1, 0);//texture);
	glUniform2i(2, W, H);
	glUniform2f(3, puzzle.sx, puzzle.sy);
	GL_CHECK;

	// uncomment this call to draw in wireframe polygons.
	//glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
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
	float X0, X1, Y0, Y1;
	puzzle.bbox(X0, X1, Y0, Y1, true);
	constexpr float F = 1.5f;
	float w = F*puzzle.sx;
	float h = F*puzzle.sy;
	int nx = std::ceil((X1-X0)/w);
	int ny = std::ceil((Y1-Y0)/h);
	if (nx == 0 && ny == 0)
	{
		X0 = X1 = camera.center.x;
		Y0 = Y1 = camera.center.y;
	}

	X0 -= (nx*w - (X1-X0))*0.5f; X0 += 0.5f*(w-puzzle.sx);
	Y0 -= (ny*h - (Y1-Y0))*0.5f; Y0 += 0.5f*(h-puzzle.sy);

	int x0 = 0, x1 = nx, y0 = 0, y1 = ny;
	int ix = 0, iy = -1, dx = 1, dy = 0;
	
	int nb = (nx < 2 || ny < 2) ? 2 : 1;
	x0 -= nb; x1 += nb; y0 -= nb; y1 += nb; ix -= nb; iy -= nb;
	nx += 2*nb; ny += 2*nb;
	//if (nx == 0) nx = 1;

	for (int i : puzzle.z)
	{
		if (puzzle.g[i] >= 0) continue;
		float x = X0 + ix * w;
		float y = Y0 + iy * h;
		puzzle.move(i, x, y, true);
		ix += dx;
		iy += dy;
		if      (dx ==  1 && ix == x1  ) { dx =  0; dy =  1; --y0; }
		else if (dy ==  1 && iy == y1  ) { dx = -1; dy =  0; ++x1; }
		else if (dx == -1 && ix == x0-1) { dx =  0; dy = -1; ++y1; }
		else if (dy == -1 && iy == y0-1) { dx =  1; dy =  0; --x0; }
	}
}

static void edge_bbox(const Puzzle &puzzle, float &x0, float &x1, float &y0, float &y1)
{
	int i = 0, N = puzzle.N;
	while (i < N && puzzle.is_edge_piece(i)) ++i;
	if (i >= N) { x0 = x1 = y0 = y1 = 0.0; return; }
	x0 = x1 = puzzle.pos[i].x;
	y0 = y1 = puzzle.pos[i].y;
	for (++i; i < N; ++i)
	{
		if (puzzle.is_edge_piece(i)) continue;
		const P2f &p = puzzle.pos[i];
		if (p.x < x0) x0 = p.x;
		if (p.x > x1) x1 = p.x;
		if (p.y < y0) y0 = p.y;
		if (p.y > y1) y1 = p.y;
	}
	x1 += 1.0f;
	y1 += 1.0f;
	x0 *= puzzle.sx; x1 *= puzzle.sx;
	y0 *= puzzle.sy; y1 *= puzzle.sy;
}

void Document::arrange_edges()
{
	if (puzzle.W < 2 || puzzle.H < 2) { arrange(); return; }
	float X0, X1, Y0, Y1;
	edge_bbox(puzzle, X0, X1, Y0, Y1);
	constexpr float F = 1.5f;
	float w = F*puzzle.sx;
	float h = F*puzzle.sy;
	int W = puzzle.W, H = puzzle.H, N = puzzle.N;
	int nx = std::ceil((X1-X0)/w);
	int ny = std::ceil((Y1-Y0)/h);
	nx = std::max(nx, (int)std::ceil(std::sqrt(W))) | 1;
	ny = std::max(ny, (int)std::ceil(std::sqrt(H))) | 1;
	float Xm = (X0+X1)*0.5f, Ym = (Y0+Y1)*0.5f;
	X0 = Xm - (nx+2)*w*0.5f;
	X1 = Xm + (nx+2)*w*0.5f;
	Y0 = Ym - (ny+2)*h*0.5f;
	Y1 = Ym + (ny+2)*h*0.5f;
	float spcx = 0.5f*(w-puzzle.sx);
	float spcy = 0.5f*(h-puzzle.sy);

	if (puzzle.g[0]   < 0) puzzle.move(  0, X0-w+spcx, Y0-h+spcy, true);
	if (puzzle.g[W-1] < 0) puzzle.move(W-1, X1+spcx,   Y0-h+spcy, true);
	if (puzzle.g[N-W] < 0) puzzle.move(N-W, X0-w+spcx, Y1+spcy,   true);
	if (puzzle.g[N-1] < 0) puzzle.move(N-1, X1+spcx,   Y1+spcy,   true);

	std::vector<int> P;
	for (int i = 1; i < W-1; ++i) if (puzzle.g[i] < 0) P.push_back(i);
	std::sort(P.begin(), P.end(), [this](int a, int b) { return puzzle.z[a] > puzzle.z[b]; });
	float d0 = (P.size() < nx && !(P.size() & 1) ? -0.5f*w : 0.0f);
	for (int n = 0; n < (int)P.size(); ++n)
	{
		int i = P[n];
		int x = n%nx + 1, y = n / nx;
		x /= (x&1 ? -2 : 2);
		puzzle.move(i, Xm-0.5*w+spcx+w*x+d0, Y0-h+spcy-y*h, true);
	}

	P.clear();
	for (int i = (H-1)*W+1; i < N-1; ++i) if (puzzle.g[i] < 0) P.push_back(i);
	std::sort(P.begin(), P.end(), [this](int a, int b) { return puzzle.z[a] > puzzle.z[b]; });
	d0 = (P.size() < nx && !(P.size() & 1) ? -0.5f*w : 0.0f);
	for (int n = 0; n < (int)P.size(); ++n)
	{
		int i = P[n];
		int x = n%nx + 1, y = n / nx;
		x /= (x&1 ? -2 : 2);
		puzzle.move(i, Xm-0.5*w+spcx+w*x+d0, Y1+spcy-y*h, true);
	}

	P.clear();
	for (int i = W; i < (H-1)*W; i += W) if (puzzle.g[i] < 0) P.push_back(i);
	std::sort(P.begin(), P.end(), [this](int a, int b) { return puzzle.z[a] > puzzle.z[b]; });
	d0 = (P.size() < ny && !(P.size() & 1) ? -0.5f*h : 0.0f);
	for (int n = 0; n < (int)P.size(); ++n)
	{
		int i = P[n];
		int y = n%ny + 1, x = n / ny;
		y /= (y&1 ? -2 : 2);
		puzzle.move(i, X0-w+spcx-x*w, Ym-0.5*h+spcy+h*y+d0, true);
	}

	P.clear();
	for (int i = W+W-1; i < N-1; i += W) if (puzzle.g[i] < 0) P.push_back(i);
	std::sort(P.begin(), P.end(), [this](int a, int b) { return puzzle.z[a] > puzzle.z[b]; });
	d0 = (P.size() < ny && !(P.size() & 1) ? -0.5f*h : 0.0f);
	for (int n = 0; n < (int)P.size(); ++n)
	{
		int i = P[n];
		int y = n%ny + 1, x = n / ny;
		y /= (y&1 ? -2 : 2);
		puzzle.move(i, X1+spcx+x*w, Ym-0.5*h+spcy+h*y+d0, true);
	}
}

int Document::hit_test(int mx, int my, bool pick_up, P2f &rel)
{
	auto p = camera.convert(mx, my, true);
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
	auto p = camera.convert(mx, my, true);
	p -= rel;
	p.x /= puzzle.sx;
	p.y /= puzzle.sy;
	puzzle.move(piece, p, false);
}
bool Document::drop(int piece)
{
	if (piece < 0 || piece >= puzzle.N) return false;
	return puzzle.connect(piece, std::max(5.0*camera.pixel_size(), 0.3));
}
