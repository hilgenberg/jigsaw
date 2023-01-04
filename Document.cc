#include "Document.h"
#include "Persistence/Serializer.h"
#include "shaders.h"
#include "Utility/GL_Util.h"
#include "Utility/GL_Program.h"
#include "Utility/Histogram.h"
#include "Utility/Preferences.h"

//------------------------------------------------------------------
// construction and drawing
//------------------------------------------------------------------

static GL_Program program;
static GL_Program bg_program;
static GLuint VBO[2] = {0,0}, VAO[3] = {0,0,0}; // use double buffering for data to avoid stalls in glMap
static GLuint texture = 0;
static int    current_buf = 0;

Document::Document()
: bg(0.25f)
{
	bg_program.add_uniform("mat4",      "view");
	bg_program.add_uniform("vec2",      "size"); // size of area
	bg_program.add_uniform("sampler2D", "image");
	bg_program.add_uniform("float",     "alpha");
	bg_program.add_shaders(bg_vertex, NULL, bg_fragment_checker);
	bg_program.add_shaders(NULL,      NULL, bg_fragment_image);

	program.add_uniform("mat4",      "view"); // camera matrix
	program.add_uniform("sampler2D", "image"); // puzzle image
	program.add_uniform("ivec2",     "count"); // puzzle.W, puzzle.H
	program.add_uniform("vec2",      "size"); // size of piece, p.x or p.y will be 1.0, the other greater than
	program.add_uniform("vec2",      "overhang"); // how far do the pieces stick out from the border? (out_border,in_border)
	program.add_shaders(vertex, geometry, fragment_none);    assert(None     == 0);
	program.add_shaders(NULL,   NULL,     fragment_regular); assert(Regular  == 1);
	program.add_shaders(NULL,   NULL,     fragment_tri);     assert(Triangle == 2);
	program.add_shaders(NULL,   NULL,     fragment_rect);    assert(Groove   == 3);
	program.add_shaders(NULL,   NULL,     fragment_circle);  assert(Circle   == 4);
}

bool Document::load(const std::string &p, int N)
{
	if (N <= 0) N = Preferences::pieces();
	GL_Image im2; if (!im2.load(p)) return false;
	free_image_data();
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
	free_image_data();
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
	#define VERTEX_DATA_SIZE N*(4*sizeof(float) + 1) /* needed below for the GLES version */
	for (int i = 0; i < 2; ++i)
	{
		assert(sizeof(Puzzle::Border) == 1);
		glBindVertexArray(VAO[i]);
		glBindBuffer(GL_ARRAY_BUFFER, VBO[i]);
		glBufferData(GL_ARRAY_BUFFER, VERTEX_DATA_SIZE, NULL, GL_DYNAMIC_DRAW);
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
void Document::free_image_data()
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
	//---------------------------------------------------------------------
	// Setup
	//---------------------------------------------------------------------

	GL_CHECK;
	if (camera.empty()) return;

	// clear background
	bg.set_clear();
	#ifdef LINUX
	glClearDepth(1.0);
	#else
	glClearDepthf(1.0f);
	#endif
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	GL_CHECK;

	//---------------------------------------------------------------------
	// draw assembly area
	//---------------------------------------------------------------------

	const bool use_image = Preferences::solution_alpha() > 1.0e-5f;
	glDepthMask(GL_FALSE);
	glDisable(GL_DEPTH_TEST);
	bg_program.use(use_image);
	glBindVertexArray(VAO[2]); // empty but needed
	if (use_image)
	{
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, texture);
		bg_program.uniform(2, 0); // wants the texture unit, not the texture ID!
		bg_program.uniform(3, Preferences::solution_alpha());
		GL_CHECK;
	}
	bg_program.uniform(0, camera.matrix());
	bg_program.uniform(1, puzzle.W*puzzle.sx, puzzle.H*puzzle.sy);
	GL_CHECK;
	//glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
	glBindVertexArray(0);
	bg_program.finish();
	GL_CHECK;

	//---------------------------------------------------------------------
	// draw puzzle
	//---------------------------------------------------------------------

	const int N = puzzle.N, W = puzzle.W, H = puzzle.H;
	if (N <= 0) return;

	program.use(Preferences::edge());
	glBindVertexArray(VAO[current_buf]);
	#ifdef LINUX
	float *data = (float*)glMapNamedBuffer(VBO[current_buf], GL_WRITE_ONLY);
	#else
	glBindBuffer(GL_ARRAY_BUFFER, VBO[current_buf]);
	float *data = (float*)glMapBufferRange(GL_ARRAY_BUFFER, 0, VERTEX_DATA_SIZE, GL_MAP_WRITE_BIT);
	#endif

	unsigned char *d = (unsigned char*)(data+4*N);
	for (int i : reverse(puzzle.z))
	{
		CameraCoords p = puzzle.to_camera(puzzle.pos[i]);
		*data++ = (float)p.x; *data++ = (float)p.y;
	 	float x = i%W, y = i/W;
		*data++ = x/W; *data++ = y/H;
		*d++ = (int)puzzle.borders[i];
	}
	GL_CHECK;
	#ifdef LINUX
	glUnmapNamedBuffer(VBO[current_buf]);
	#else
	glUnmapBuffer(GL_ARRAY_BUFFER);
	#endif

	float  d1 = 0.0f, d0 = 0.0f;
	Puzzle::overhang(Preferences::edge(), d1, d0);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, texture);
	program.uniform(0, camera.matrix());
	program.uniform(1, 0); // wants the texture unit, not the texture ID!
	program.uniform(2, W, H);
	program.uniform(3, puzzle.sx, puzzle.sy);
	program.uniform(4, d1, d0);
	GL_CHECK;

	glDepthMask(GL_TRUE);
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS);
	glDrawArrays(GL_POINTS, 0, N);
	GL_CHECK;

	glBindVertexArray(0);
	program.finish();

	++current_buf; current_buf %= 2;
}

Document::~Document()
{
	free_image_data();
}

void Document::reset_view()
{
	double x0, x1, y0, y1;
	puzzle.bbox(x0, x1, y0, y1);
	x0 = std::min(x0, -0.5*puzzle.W*puzzle.sx);
	x1 = std::max(x1,  0.5*puzzle.W*puzzle.sx);
	y0 = std::min(y0, -0.5*puzzle.H*puzzle.sy);
	y1 = std::max(y1,  0.5*puzzle.H*puzzle.sy);
	camera.view_box(x0, x1, y0, y1, 1.25);
}

void Document::drag_view(const ScreenCoords &d)
{
	camera.move(camera.from_screen(d));
}
void Document::zoom(float factor, const ScreenCoords &center)
{
	camera.zoom(factor, camera.from_screen(center));
}
