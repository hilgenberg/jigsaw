#include "Buttons.h"
#include "data.h"
#include "shaders.h"
#include "Utility/GL_Util.h"
#include "Utility/GL_Program.h"
#include "Utility/Preferences.h"
#include "Window.h"

static GL_Program program;
static GLuint VBO[2] = {0,0}, VAO[2] = {0,0}; // use double buffering for data to avoid stalls in glMap
static GLuint texture = 0;
static int    current_buf = 0;
static constexpr int MAX_BUTTONS = N_IMAGES;
static P3f button_size(0, 0, 0); // (w,h,spacing)

bool Buttons::Button::hit(const Window &w, int mx, int my) const
{
	P2<int> s = w.size();
	float x = 2.0f*mx / s.x - 1.0f, y = 1.0f - 2.0f*my / s.y;
	return fabs(x-pos.x) < button_size.x*0.5f &&
	fabs(y-pos.y) < button_size.y*0.5f;
}

Buttons::Buttons(Window &win) : window(win)
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

	program.add_uniform("sampler2D", "image");
	program.add_uniform("vec2",      "size"); // size of buttons
	program.add_uniform("int",       "n_buttons"); // number of buttons in the texture
	program.add_shaders(buttons_vertex, buttons_geometry, buttons_fragment);

	glGenVertexArrays(2, VAO);
	glGenBuffers(2, VBO);
	#define VERTEX_DATA_SIZE MAX_BUTTONS*(2*sizeof(float) + 2) /* needed below for the GLES version */
	for (int i = 0; i < 2; ++i)
	{
		glBindVertexArray(VAO[i]);
		glBindBuffer(GL_ARRAY_BUFFER, VBO[i]);
		glBufferData(GL_ARRAY_BUFFER, VERTEX_DATA_SIZE, NULL, GL_DYNAMIC_DRAW);
		glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2*sizeof(float), (void*)0); // pos
		glVertexAttribIPointer(1, 1, GL_UNSIGNED_BYTE,  2, (void*)(2*sizeof(float)*MAX_BUTTONS)); // button image index
		glVertexAttribIPointer(2, 1, GL_UNSIGNED_BYTE,  2, (void*)(2*sizeof(float)*MAX_BUTTONS + 1)); // active
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

	reshape();
}

Buttons::~Buttons()
{
	glDeleteVertexArrays(2, VAO); VAO[0] = VAO[1] = 0;
	glDeleteBuffers(2, VBO); VBO[0] = VBO[1] = 0;
	glDeleteTextures(1, &texture); texture = 0;
}

void Buttons::reshape()
{
	P2<int> size = window.size();
	int W = size.x, H = size.y;
	if (W <= 0) W = 1;
	if (H <= 0) H = 1;
	float w = 0.15f * exp(3.0*Preferences::button_scale()), h = w, spc = h*0.5f;
	if (W < H) h = w*W/H; else w = h*H/W;
	const int nspc = 2, nb = 7;
	P2f p(0.0, 0.0), dp(0.0, 0.0), ds(0.0, 0.0);

	switch (Preferences::button_edge())
	{
		case LEFT:
		case RIGHT:
		{
			w = h*H/W; if (w > 2.0f) { w = 2.0f; h = w*W/H; }

			if (nb*h + nspc*spc > 2.0f)
			{
				if (nspc > 0) spc = (2.0f - nb*h)/nspc; else spc = 0.0f;
				if (spc < 0.0f) spc = 0.0f;
			}

			if (nb*h + nspc*spc > 2.0f)
			{
				assert(spc == 0.0f);
				h = 2.0f/nb;
				w = h*H/W; assert(w <= 2.0f);
			}

			p.x = Preferences::button_edge() == LEFT ? -1.0+w*0.5f : 1.0-w*0.5f;
			dp.set(0.0, -h);
			ds.set(0.0, -spc);
			switch (Preferences::button_align())
			{
				case TOP_OR_LEFT: p.y = 1.0f-h*0.5f; break;
				case CENTERED: p.y = 0.5*(nb*h + nspc*spc)-h*0.5f; break;
				case BOTTOM_OR_RIGHT: p.y = -1.0f-h*0.5f + nb*h + nspc*spc; break;
			}
			break;
		}
		case TOP:
		case BOTTOM:
		{
			h = w*W/H; if (h > 2.0f) { h = 2.0f; w = h*H/W; }

			if (nb*w + nspc*spc > 2.0f)
			{
				if (nspc > 0) spc = (2.0f - nb*w)/nspc; else spc = 0.0f;
				if (spc < 0.0f) spc = 0.0f;
			}

			if (nb*w + nspc*spc > 2.0f)
			{
				assert(spc == 0.0f);
				w = 2.0f/nb;
				h = w*W/H; assert(h <= 2.0f);
			}

			p.y = Preferences::button_edge() == TOP ? 1.0-h*0.5f : -1.0+h*0.5f;
			dp.set(w, 0.0);
			ds.set(spc, 0.0);
			switch (Preferences::button_align())
			{
				case TOP_OR_LEFT: p.x = -1.0f+w*0.5f; break;
				case CENTERED: p.x = -0.5*(nb*w + nspc*spc)+w*0.5f; break;
				case BOTTOM_OR_RIGHT: p.x = 1.0f+w*0.5f - nb*w - nspc*spc; break;
			}
			break;
		}

		default: assert(false);
	}

	button_size.set(w, h, spc);
	buttons.resize(nb);
	auto *b = &buttons[-1];

	(++b)->index = RESET_VIEW;   b->pos = p; p += dp;
	(++b)->index = ARRANGE;      b->pos = p; p += dp;
	(++b)->index = EDGE_ARRANGE; b->pos = p; p += dp;
	p += ds;
	(++b)->index = SETTINGS; b->pos = p; p += dp;
	p += ds;
	(++b)->index = HIDE;   b->pos = p; p += dp;
	(++b)->index = SHOVEL; b->pos = p; p += dp;
	(++b)->index = MAGNET; b->pos = p; p += dp;
}

void Buttons::draw()
{
	if (buttons.empty()) return;
	assert((int)buttons.size() <= MAX_BUTTONS);

	program.use();
	glBindVertexArray(VAO[current_buf]);
	#ifdef LINUX
	float *data = (float*)glMapNamedBuffer(VBO[current_buf], GL_WRITE_ONLY);
	#else
	glBindBuffer(GL_ARRAY_BUFFER, VBO[current_buf]);
	float *data = (float*)glMapBufferRange(GL_ARRAY_BUFFER, 0, VERTEX_DATA_SIZE, GL_MAP_WRITE_BIT);
	#endif

	unsigned char *d = (unsigned char*)(data+2*MAX_BUTTONS);
	for (const auto &b : buttons)
	{
		*d++ = b.index;
		switch (b.index)
		{
			case HIDE:   *d++ = (window.active_tool() == Tool::HIDE);   break;
			case SHOVEL: *d++ = (window.active_tool() == Tool::SHOVEL); break;
			case MAGNET: *d++ = (window.active_tool() == Tool::MAGNET); break;
			default: *d++ = true; break;
		}
		*data++ = b.pos.x;
		*data++ = b.pos.y;
	}
	GL_CHECK;

	#ifdef LINUX
	glUnmapNamedBuffer(VBO[current_buf]);
	#else
	glUnmapBuffer(GL_ARRAY_BUFFER);
	#endif

	// glDepthMask(GL_FALSE); <-- triggers some GPU bug that kills all drawing, no idea why...

	glDisable(GL_DEPTH_TEST);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, texture);
	program.uniform(0, 1); // wants the texture unit, not the texture ID!
	program.uniform(1, button_size.x, button_size.y);
	program.uniform(2, N_IMAGES); // number of buttons in the texture
	GL_CHECK;

	//glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	glDrawArrays(GL_POINTS, 0, (int)buttons.size());
	GL_CHECK;
	glBindVertexArray(0); 
	program.finish();

	++current_buf; current_buf %= 2;
}
