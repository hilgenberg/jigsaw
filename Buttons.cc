#include "Buttons.h"
#include "data.h"
#include "shaders.h"
#include "Utility/GL_Util.h"
#include "Utility/Preferences.h"
#include "Window.h"
#include <GL/gl.h>
#include <GL/glu.h>

static GLuint program = 0;
static GLuint VBO[2] = {0,0}, VAO[2] = {0,0}; // use double buffering for data to avoid stalls in glMap
static GLuint texture = 0;
static int    current_buf = 0;
static constexpr int MAX_BUTTONS = N_IMAGES;
static P2f button_size(0.0f, 0.0f);
static P2f foldout_direction(1.0f, 0.0f);

struct Button
{
	ButtonAction index; // index into button texture
	P2f pos; // center position
	float alpha() const { return 1.0f; }
	bool hit(const Window &w, int mx, int my) const
	{
		P2<int> s = w.size();
		float x = 2.0f*mx / s.x - 1.0f, y = 1.0f - 2.0f*my / s.y;
		return fabs(x-pos.x) < button_size.x*0.5f &&
		       fabs(y-pos.y) < button_size.y*0.5f;
	}
};
static std::vector<Button> buttons;

Buttons::Buttons(Window &win) : w(win)
{
	//--- load images -----------------------------------------------------
	GL_Image im;
	{
		std::vector<GL_Image> tmp(N_IMAGES);
		tmp[ARRANGE].load(ic_arrange_data());
		tmp[EDGE_ARRANGE].load(ic_edge_data());
		tmp[RESET_VIEW].load(ic_view_data());
		tmp[HIDE].load(ic_hide_data());
		tmp[SHOVEL].load(ic_shovel_data());
		tmp[MAGNET].load(ic_magnet_data());
		tmp[SETTINGS].load(ic_settings_data());
		
		int w = tmp[0].w(), h = tmp[0].h();
		#ifndef NDEBUG
		assert(w == h);
		for (const auto &im : tmp) { assert(im.w() == w); assert(im.h() == h); }
		#endif

		auto *d = im.redim(w, h * N_IMAGES);
		for (const auto &t : tmp)
		{
			memcpy(d, t.data().data(), w*h*4);
			d += w*h*4;
		}
	}

	//--- put into texture ------------------------------------------------

	assert(!texture);
	glGenTextures(1, &texture);
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, texture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, im.w(), im.h(), 0, GL_RGBA, GL_UNSIGNED_BYTE, im.data().data());
	glGenerateMipmap(GL_TEXTURE_2D);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
	float borderColor[] = { 0.0f, 0.0f, 0.0f, 0.0f };
	glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	GL_CHECK;

	//--- setup GL --------------------------------------------------------

	std::map<GLuint, const char *> shaders = {
		{GL_VERTEX_SHADER,   buttons_vertex},
		{GL_GEOMETRY_SHADER, buttons_geometry},
		{GL_FRAGMENT_SHADER, buttons_fragment}
	};
	program = compileShaders(shaders);
	GL_CHECK;

	glGenVertexArrays(2, VAO);
	glGenBuffers(2, VBO);
	for (int i = 0; i < 2; ++i)
	{
		glBindVertexArray(VAO[i]);
		glBindBuffer(GL_ARRAY_BUFFER, VBO[i]);
		glBufferData(GL_ARRAY_BUFFER, MAX_BUTTONS*(3*sizeof(float) + 1), NULL, GL_DYNAMIC_DRAW);
		glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 3*sizeof(float), (void*)0); // pos
		glVertexAttribPointer(1, 1, GL_FLOAT, GL_FALSE, 3*sizeof(float), (void*)(2*sizeof(float))); // alpha
		glVertexAttribIPointer(2, 1, GL_UNSIGNED_BYTE,  0, (void*)(3*sizeof(float)*MAX_BUTTONS)); // button image index
		glEnableVertexAttribArray(0);
		glEnableVertexAttribArray(1);
		glEnableVertexAttribArray(2);
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, texture);
		glBindBuffer(GL_ARRAY_BUFFER, 0); 
		GL_CHECK;
	}
	glBindVertexArray(0); 
	GL_CHECK;

	//--- initial button placement ----------------------------------------

	P2<int> size = w.size();
	reshape(size.x, size.y);
}

Buttons::~Buttons()
{
	glDeleteVertexArrays(2, VAO); VAO[0] = VAO[1] = 0;
	glDeleteBuffers(2, VBO); VBO[0] = VBO[1] = 0;
	glDeleteTextures(1, &texture); texture = 0;
	glDeleteProgram(program); program = 0;
}

void Buttons::reshape(int W, int H)
{
	float w = 0.15f, h = w;
	if (W < H) h = w*W/H; else w = h*H/W;
	button_size.set(w, h);

	buttons.resize(N_IMAGES);
	auto *b = &buttons[-1];
	float x = -1.0+w*0.5f, y = 1.0f-h*0.5f;

	(++b)->index = RESET_VIEW;
	b->pos.set(x, y); y -= h;

	(++b)->index = ARRANGE;
	b->pos.set(x, y); y -= h;

	(++b)->index = EDGE_ARRANGE;
	b->pos.set(x, y); y -= h;

	(++b)->index = SETTINGS;
	b->pos.set(x, y); y -= h;

	(++b)->index = HIDE;
	b->pos.set(x, y); y -= h;

	(++b)->index = SHOVEL;
	b->pos.set(x, y); y -= h;

	(++b)->index = MAGNET;
	b->pos.set(x, y); y -= h;
}

void Buttons::draw()
{
	if (buttons.empty()) return;
	assert((int)buttons.size() <= MAX_BUTTONS);

	float *data = (float*)glMapNamedBuffer(VBO[current_buf], GL_WRITE_ONLY);
	unsigned char *d = (unsigned char*)(data+3*MAX_BUTTONS);
	for (const auto &b : buttons)
	{
		*d++    = b.index;
		*data++ = b.pos.x;
		*data++ = b.pos.y;
		*data++ = b.alpha();
	}
	GL_CHECK;
	glUnmapNamedBuffer(VBO[current_buf]);

	// glDepthMask(GL_FALSE); <-- triggers some GPU bug that kills all drawing, no idea why...

	glDisable(GL_DEPTH_TEST);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	glUseProgram(program);
	glBindVertexArray(VAO[current_buf]);
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, texture);
	glUniform1i(0, 1); // wants the texture unit, not the texture ID!
	glUniform2f(1, button_size.x, button_size.y);
	glUniform1i(2, N_IMAGES); // number of buttons in the texture
	GL_CHECK;

	//glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	glDrawArrays(GL_POINTS, 0, (int)buttons.size());
	GL_CHECK;
	glBindVertexArray(0); 
	glUseProgram(0);

	++current_buf; current_buf %= 2;
}

bool Buttons::handle_event(const SDL_Event &e)
{
	switch (e.type)
	{
		case SDL_MOUSEBUTTONDOWN:
		{
			if (e.button.button != SDL_BUTTON_LEFT) return false;
			for (auto &b : buttons)
			{
				if (!b.hit(w, e.button.x, e.button.y)) continue;
				w.button_action(b.index);
				return true;
			}
			return false;
		}
		case SDL_MOUSEBUTTONUP:
			if (e.button.button != SDL_BUTTON_LEFT) return false;
			return false;
		case SDL_MOUSEMOTION:
		{
			return false;

			//auto buttons = e.motion.state;
			//static int i = 0; ++i; i %=10;
			//std::cout << i << "B " << buttons << std::endl;
			//if (!(buttons & (SDL_BUTTON_LMASK|SDL_BUTTON_RMASK|SDL_BUTTON_MMASK))) return true;
			
			//double dx = e.motion.xrel, dy = e.motion.yrel, dz = 0.0;
				//doc.drag(dragging, drag_rel, e.motion.x, e.motion.y, drag_v, dx, dy);
		}
	}
	return false;
}
