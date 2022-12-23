#include "Document.h"
#include "../Persistence/Serializer.h"
#include "shaders.h"
#include "Utility/GL_Util.h"
#include "Utility/Histogram.h"
#include "Utility/Preferences.h"
#include <GL/gl.h>
#include <GL/glu.h>

//------------------------------------------------------------------
// construction and drawing
//------------------------------------------------------------------

static GLuint program = 0;
static GLuint bg_program[2] = {0};
static GLuint VBO[2] = {0,0}, VAO[2] = {0,0}; // use double buffering for data to avoid stalls in glMap
static GLuint texture = 0;
static int    current_buf = 0;
static int    loaded_edge = -1; // which edge type is in program?
static float  d1 = 0.0f, d0 = 0.0f; // for the "overhang" uniform

Document::Document()
: bg(0.25f)
{
}

bool Document::load(const std::string &p, int N)
{
	if (N <= 0) N = Preferences::pieces();
	GL_Image im2; if (!im2.load(p)) return false;
	free_all();
	im_path = p;
	im.swap(im2);
	histo = nullptr;
	puzzle.reset(im.w(), im.h(), N);
	puzzle.shuffle(false);
	reset_view();
	init();
	return true;
}

void Document::load(Deserializer &s)
{
	free_all();
	s.string_(im_path);
	s.member_(puzzle);
	s.member_(camera);
	s.marker_("EOF.");
	im.load(im_path); histo = nullptr;
	if (puzzle.solved())
	{
		puzzle.reset(im.w(), im.h(), puzzle.N);
		puzzle.shuffle(false);
	}
	init();
}
void Document::save(Serializer &s) const
{
	s.string_(im_path);
	s.member_(puzzle);
	s.member_(camera);
	s.marker_("EOF.");
}

void Document::init()
{
	if (!bg_program[0])
	{
		std::map<GLuint, const char *> shaders = {
			{GL_VERTEX_SHADER,   bg_vertex},
			{GL_FRAGMENT_SHADER, bg_fragment_checker}
		};
		bg_program[0] = compileShaders(shaders);
		GL_CHECK;

		 shaders = std::map<GLuint, const char *>{
			{GL_VERTEX_SHADER,   bg_vertex},
			{GL_FRAGMENT_SHADER, bg_fragment_image}
		};
		bg_program[1] = compileShaders(shaders);
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
		glBufferData(GL_ARRAY_BUFFER, N*(4*sizeof(float) + 1), NULL, GL_DYNAMIC_DRAW);
		glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4*sizeof(float), (void*)0); // pos
		glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4*sizeof(float), (void*)(2*sizeof(float))); // tex
		glVertexAttribIPointer(2, 1, GL_UNSIGNED_BYTE,  0, (void*)(4*sizeof(float)*N)); // border
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

template<typename T> struct Reversed             { T& it; };
template<typename T> auto   begin(Reversed<T> w) { return std::rbegin(w.it); }
template<typename T> auto     end(Reversed<T> w) { return std::rend(w.it); }
template<typename T> Reversed<T> reverse(T&& it) { return {it}; }

void Document::draw()
{
	GL_CHECK;
	if (camera.empty()) return;

	if (!program || loaded_edge != Preferences::edge())
	{
		if (program) { glDeleteProgram(program); program = 0; }

		const char *frag = NULL;
		switch (loaded_edge = Preferences::edge())
		{
			case None:     frag = fragment_none; break;
			case Regular:  frag = fragment_regular; break;
			case Triangle: frag = fragment_tri; break;
			case Groove:   frag = fragment_rect; break;
			case Circle:   frag = fragment_circle; break;
			default: assert(false);
		}
		Puzzle::overhang((EdgeType)loaded_edge, d1, d0);

		std::map<GLuint, const char *> shaders = {
			{GL_VERTEX_SHADER,   vertex},
			{GL_GEOMETRY_SHADER, geometry},
			{GL_FRAGMENT_SHADER, frag}
		};
		program = compileShaders(shaders);
		GL_CHECK;
	}

	// clear background
	bg.set_clear();
	glClearDepth(1.0);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	GL_CHECK;

	// draw assembly area
	const bool use_image = Preferences::solution_alpha() > 1.0e-5f;
	glDepthMask(GL_FALSE);
	glDisable(GL_DEPTH_TEST);
	glUseProgram(bg_program[use_image]);
	glBindVertexArray(VAO[2]); // empty but needed
	if (use_image)
	{
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, texture);
		glUniform1i(2, 0); // wants the texture unit, not the texture ID!
		glUniform1f(3, Preferences::solution_alpha());
	}
	camera.set(0);
	glUniform2f(1, puzzle.W*puzzle.sx, puzzle.H*puzzle.sy);
	//glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
	glBindVertexArray(0);
	GL_CHECK;

	// send current data
	GL_CHECK;
	const int N = puzzle.N, W = puzzle.W, H = puzzle.H;
	if (N <= 0) return;
	const float sx = puzzle.sx, sy = puzzle.sy;
	float *data = (float*)glMapNamedBuffer(VBO[current_buf], GL_WRITE_ONLY);
	unsigned char *d = (unsigned char*)(data+4*N);
	for (int i : reverse(puzzle.z))
	{
		const P2f &p = puzzle.pos[i];
		*data++ = p.x*sx; *data++ = p.y*sy;
	 	float x = i%W, y = i/W;
		*data++ = x/W; *data++ = y/H;
		*d++ = (int)puzzle.borders[i];
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
	glUniform2f(4, d1, d0);
	GL_CHECK;

	glDepthMask(GL_TRUE);
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS);
	glDrawArrays(GL_POINTS, 0, N);
	GL_CHECK;

	++current_buf; current_buf %= 2;
}

Document::~Document()
{
	free_all();
	if (program) glDeleteProgram(program); program = 0;
	if (bg_program[0]) glDeleteProgram(bg_program[0]); bg_program[0] = 0;
	if (bg_program[1]) glDeleteProgram(bg_program[1]); bg_program[1] = 0;
}

void Document::reset_view()
{
	float x0, x1, y0, y1;
	puzzle.bbox(x0, x1, y0, y1);
	x0 = std::min(x0, -0.5f*puzzle.W*puzzle.sx);
	x1 = std::max(x1,  0.5f*puzzle.W*puzzle.sx);
	y0 = std::min(y0, -0.5f*puzzle.H*puzzle.sy);
	y1 = std::max(y1,  0.5f*puzzle.H*puzzle.sy);
	camera.view_box(x0, x1, y0, y1, 1.25f);
}
