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

static GLuint program;
static GLuint VBO[2], VAO[2]; // use double buffering for data to avoid stalls in glMap
static int current_buf = 0;
static GLuint texture; 

Document::Document(const std::string &im_path, int N)
: bg(0.25f)
{
	if (N <= 0) N = 50;
	im.load(im_path);
	puzzle.reset(im.w(), im.h(), N);
	N = puzzle.N;
	puzzle.shuffle(false);
	reset_view();

	std::map<GLuint, const char *> shaders = {
		{GL_VERTEX_SHADER,   vertex},
		{GL_GEOMETRY_SHADER, geometry},
		{GL_FRAGMENT_SHADER, fragment}
	};
	program = compileShaders(shaders);
	GL_CHECK;

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
	glDeleteVertexArrays(2, VAO);
	glDeleteBuffers(2, VBO);
	glDeleteProgram(program);
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
		puzzle.pos[i].set(x/puzzle.sx, y/puzzle.sy);
		ix += dx;
		iy += dy;
		if      (dx ==  1 && ix == x1  ) { dx =  0; dy =  1; --y0; }
		else if (dy ==  1 && iy == y1  ) { dx = -1; dy =  0; ++x1; }
		else if (dx == -1 && ix == x0-1) { dx =  0; dy = -1; ++y1; }
		else if (dy == -1 && iy == y0-1) { dx =  1; dy =  0; --x0; }
	}
}

void Document::arrange_edges()
{
	
}

int Document::hit_test(int mx, int my, bool pick_up)
{
	auto p = camera.convert(mx, my, true);
	p.x /= puzzle.sx;
	p.y /= puzzle.sy;
	int i = puzzle.hit_test(p);
	if (pick_up && i >= 0) puzzle.pick_up(i);
	return i;
}
void Document::drag(int piece, int dx, int dy)
{
	if (piece < 0 || piece >= puzzle.N) return;
	auto p = camera.convert(dx, dy, false);
	p.x /= puzzle.sx;
	p.y /= puzzle.sy;
	puzzle.move(piece, puzzle.pos[piece] + p);
}
void Document::drop(int piece)
{
	if (piece < 0 || piece >= puzzle.N) return;
	puzzle.connect(piece, 5.0*camera.pixel_size());
}

//------------------------------------------------------------------
// persistence
//------------------------------------------------------------------

void Document::saveAs(const std::string &p) const
{
	FILE *F = fopen(p.c_str(), "w");
	if (!F) throw std::runtime_error(std::string("can't open file for writing: ") + p);
	try
	{
		FileWriter fw(F);
		Serializer  w(fw);
		puzzle.save(w);
		w.marker_("EOF.");
	}
	catch(...)
	{
		fclose(F);
		throw;
	}
	fclose(F);
}

void Document::load(const std::string &p)
{
	FILE *F = fopen(p.c_str(), "r");
	if (!F) throw std::runtime_error(std::string("can't open file for reading: ") + p);
	try
	{
		FileReader   fr(F);
		Deserializer s(fr);
		puzzle.load(s);
		s.marker_("EOF.");
		assert(s.done());
	}
	catch(...)
	{
		fclose(F);
		throw;
	}
	fclose(F);
}
