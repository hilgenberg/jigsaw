#include "Renderer.h"
#include "Document.h"
#include "Utility/GL_Util.h"
#include "data.h"
#include "shaders.h"
#include "Window.h"
#include "GUI.h"
#include "imgui/imgui.h"

#ifdef LINUX
#include "imgui/backends/imgui_impl_sdl.h"
#include "imgui/backends/imgui_impl_opengl3.h"
#else
#include <android/native_window_jni.h>
#include "imgui/backends/imgui_impl_android.h"
#define IMGUI_IMPL_OPENGL_ES3
#include "imgui/backends/imgui_impl_opengl3.h"
#endif

static constexpr int MAX_BUTTONS = N_BUTTON_IMAGES;

#ifdef LINUX
Renderer::Renderer(Document &doc, Window &window, GUI &gui, SDL_Window *sdl_window, SDL_GLContext sdl_context)
#else
Renderer::Renderer(Document &doc, Window &window, GUI &gui, ANativeWindow *jni_window)
#endif
: doc(doc)
, window(window)
, gui(gui)
#ifdef ANDROID
, context(eglGetCurrentContext())
, jni_window(jni_window)
#endif
{
	#ifdef ANDROID
	assert(context != EGL_NO_CONTEXT);
	#endif
	GL_CHECK;

	//----------------------------------------------------------------------------
	// setup textures
	//----------------------------------------------------------------------------

	/* puzzle image */ {
		const GL_Image &im = doc.puzzle.im;
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
	}

	/* button images */ {
		GL_Image im;
		std::vector<GL_Image> tmp(N_BUTTON_IMAGES);
		#define ICON(id, x) tmp[id].load(x##_data, x##_data_len)
		ICON(ARRANGE,      ic_arrange);
		ICON(EDGE_ARRANGE, ic_edge);
		ICON(RESET_VIEW,   ic_view);
		ICON(HIDE,         ic_hide);
		ICON(SHOVEL,       ic_shovel);
		ICON(MAGNET,       ic_magnet);
		ICON(CHANGE_IMAGE, ic_change_image);
		ICON(SETTINGS,     ic_settings);
		ICON(PREFERENCES,  ic_preferences);
		#undef ICON
		
		int w = tmp[0].w(), h = tmp[0].h();
		#ifndef NDEBUG
		assert(w == h);
		for (const auto &im : tmp) { assert(im.w() == w); assert(im.h() == h); }
		#endif

		auto *d = im.redim(w, h * N_BUTTON_IMAGES);
		for (const auto &t : tmp)
		{
			memcpy(d, t.data().data(), w*h*4);
			d += w*h*4;
		}

		glGenTextures(1, &button_texture);
		glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, button_texture);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, im.w(), im.h(), 0, GL_RGBA, GL_UNSIGNED_BYTE, im.data().data());
		glGenerateMipmap(GL_TEXTURE_2D);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
		float borderColor[] = { 0.0f, 0.0f, 0.0f, 0.0f };
		glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		GL_CHECK;
	}

	//----------------------------------------------------------------------------
	// program setup
	//----------------------------------------------------------------------------

	bg_program.add_uniform("mat4",      "view");
	bg_program.add_uniform("vec2",      "size"); // size of area
	bg_program.add_uniform("sampler2D", "image");
	bg_program.add_uniform("float",     "alpha");
	bg_program.add_uniform("vec4",      "bg");
	bg_program.add_uniform("vec4",      "bg_light");
	bg_program.add_uniform("vec4",      "bg_dark");
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

	button_program.add_uniform("sampler2D", "image");
	button_program.add_uniform("vec2",      "size"); // size of buttons
	button_program.add_uniform("int",       "n_buttons"); // number of buttons in the texture
	button_program.add_shaders(buttons_vertex, buttons_geometry, buttons_fragment_light);
	button_program.add_shaders(NULL,           NULL,             buttons_fragment_dark);

	//----------------------------------------------------------------------------
	// attribute allocation
	//----------------------------------------------------------------------------

	glGenVertexArrays(1, &bg_VAO);
	alloc_puzzle_VOs(false);
	glGenVertexArrays(2, button_VAO);
	glGenBuffers(2, button_VBO);
	#define BUTTON_VERTEX_DATA_SIZE MAX_BUTTONS*(2*sizeof(float) + 2) /* needed below for the GLES version */
	for (int i = 0; i < 2; ++i)
	{
		glBindVertexArray(button_VAO[i]);
		glBindBuffer(GL_ARRAY_BUFFER, button_VBO[i]);
		glBufferData(GL_ARRAY_BUFFER, BUTTON_VERTEX_DATA_SIZE, NULL, GL_DYNAMIC_DRAW);
		glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2*sizeof(float), (void*)0); // pos
		glVertexAttribIPointer(1, 1, GL_UNSIGNED_BYTE,  2, (void*)(2*sizeof(float)*MAX_BUTTONS)); // button image index
		glVertexAttribIPointer(2, 1, GL_UNSIGNED_BYTE,  2, (void*)(2*sizeof(float)*MAX_BUTTONS + 1)); // active
		glEnableVertexAttribArray(0);
		glEnableVertexAttribArray(1);
		glEnableVertexAttribArray(2);
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, button_texture);
		glBindBuffer(GL_ARRAY_BUFFER, 0); 
		GL_CHECK;
	}

	glBindVertexArray(0);

	//----------------------------------------------------------------------------
	// Dear ImGui setup
	//----------------------------------------------------------------------------

	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO &io = ImGui::GetIO();
	io.ConfigInputTextCursorBlink = false;
	io.ConfigInputTrickleEventQueue = false;
	io.IniFilename = NULL;
	#ifdef LINUX
	ImGui_ImplSDL2_InitForOpenGL(sdl_window, sdl_context);
	ImGui_ImplOpenGL3_Init();
	float font_size = 18.0f;
	#else
	ImGui_ImplAndroid_Init(jni_window);
	ImGui_ImplOpenGL3_Init("#version 300 es");
	ImGui::GetStyle().ScaleAllSizes(3.0f); // FIXME: Put some effort into DPI awareness
	float font_size = 42.0f;
	#endif

	ImFontConfig fc; fc.FontDataOwnedByAtlas = false;
	static const ImWchar ranges[] = { 0x0001, 0xFFFF, 0 }; // get all
	io.Fonts->AddFontFromMemoryTTF((void *)font_data, (int)font_data_len, font_size, &fc, ranges);

	auto &style = ImGui::GetStyle();
	style.FrameRounding = 3.0f;
	style.WindowRounding = 5.0f;
	//style.ChildRounding = 5.0f;
	#ifdef ANDROID
	style.FrameRounding *= 3.0f;
	style.WindowRounding *= 3.0f;
	#endif
}

void Renderer::alloc_puzzle_VOs(bool free_old_buffers)
{
	GL_CHECK;
	if (free_old_buffers)
	{
		glDeleteVertexArrays(2, VAO);
		glDeleteBuffers(2, VBO);
		GL_CHECK;
	}

	const int N = doc.puzzle.N;
	glGenVertexArrays(2, VAO);
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
	current_N = N;
}

Renderer::~Renderer()
{
	#ifdef LINUX
	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplSDL2_Shutdown();
	#else
	if (eglGetCurrentContext() == context) ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplAndroid_Shutdown();
	ANativeWindow_release(jni_window);
	#endif
	ImGui::DestroyContext();

	#ifdef ANDROID
	if (eglGetCurrentContext() != context)
	{
		program.drop();
		bg_program.drop();
		button_program.drop();
		return;
	}
	#endif

	glDeleteVertexArrays(2, VAO);
	glDeleteVertexArrays(1, &bg_VAO);
	glDeleteVertexArrays(2, button_VAO);
	glDeleteBuffers(2, VBO);
	glDeleteBuffers(2, button_VBO);
	glDeleteTextures(1, &texture);
	glDeleteTextures(1, &button_texture);
}


template<typename T> struct Reversed             { T& it; };
template<typename T> auto   begin(Reversed<T> w) { return std::rbegin(w.it); }
template<typename T> auto     end(Reversed<T> w) { return std::rend(w.it); }
template<typename T> Reversed<T> reverse(T&& it) { return {it}; }

void Renderer::draw_background()
{
	auto &puzzle = doc.puzzle;
	const bool use_image = Preferences::solution_alpha() > 1.0e-5f;
	glDepthMask(GL_FALSE);
	glDisable(GL_DEPTH_TEST);
	bg_program.use(use_image);
	glBindVertexArray(bg_VAO); // empty but needed
	bg_program.uniform(0, doc.camera.matrix());
	bg_program.uniform(1, puzzle.W*puzzle.sx, puzzle.H*puzzle.sy);
	bg_program.uniform(4, Preferences::bg_color());
	if (use_image)
	{
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, texture);
		bg_program.uniform(2, 0); // wants the texture unit, not the texture ID!
		bg_program.uniform(3, Preferences::solution_alpha());
		GL_CHECK;
	}
	else
	{
		GL_Color c1 = Preferences::bg_color(), c2 = c1;
		if (Preferences::dark_mode())
		{
			// lighten the colors
			c1.light_mul(0.29f/0.25f);
			c2.light_mul(0.3f/0.25f);
		}
		else
		{
			// darken the colors
			c1.light_mul(0.25f/0.29f);
			c2.light_mul(0.25f/0.3f);
		}
		bg_program.uniform(5, c1);
		bg_program.uniform(6, c2);
	}
	GL_CHECK;
	//glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
	glBindVertexArray(0);
	bg_program.finish();
	GL_CHECK;
}

void Renderer::draw_puzzle()
{
	auto &puzzle = doc.puzzle;
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
	program.uniform(0, doc.camera.matrix());
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
}

void Renderer::draw_buttons()
{
	if (doc.buttons.buttons.empty()) return;
	assert((int)doc.buttons.buttons.size() <= MAX_BUTTONS);

	button_program.use(1-Preferences::dark_mode());
	glBindVertexArray(button_VAO[current_buf]);
	#ifdef LINUX
	float *data = (float*)glMapNamedBuffer(button_VBO[current_buf], GL_WRITE_ONLY);
	#else
	glBindBuffer(GL_ARRAY_BUFFER, button_VBO[current_buf]);
	float *data = (float*)glMapBufferRange(GL_ARRAY_BUFFER, 0, BUTTON_VERTEX_DATA_SIZE, GL_MAP_WRITE_BIT);
	#endif

	unsigned char *d = (unsigned char*)(data+2*MAX_BUTTONS);
	for (const auto &b : doc.buttons.buttons)
	{
		*d++ = b.index;
		switch (b.index)
		{
			case HIDE:   *d++ = (doc.tool == Tool::HIDE);   break;
			case SHOVEL: *d++ = (doc.tool == Tool::SHOVEL); break;
			case MAGNET: *d++ = (doc.tool == Tool::MAGNET); break;
			default: *d++ = true; break;
		}
		*data++ = b.pos.x;
		*data++ = b.pos.y;
	}
	GL_CHECK;

	#ifdef LINUX
	glUnmapNamedBuffer(button_VBO[current_buf]);
	#else
	glUnmapBuffer(GL_ARRAY_BUFFER);
	#endif

	// glDepthMask(GL_FALSE); <-- triggers some GPU bug that kills all drawing, no idea why...

	glDisable(GL_DEPTH_TEST);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, button_texture);
	button_program.uniform(0, 1); // wants the texture unit, not the texture ID!
	button_program.uniform(1, doc.buttons.button_size.x, doc.buttons.button_size.y);
	button_program.uniform(2, N_BUTTON_IMAGES); // number of buttons in the texture
	GL_CHECK;

	//glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	glDrawArrays(GL_POINTS, 0, (int)doc.buttons.buttons.size());
	GL_CHECK;
	glBindVertexArray(0); 
	button_program.finish();
}

void Renderer::draw_gui()
{
	#ifdef LINUX
	if (ImGui::GetIO().MouseDrawCursor != gui.visible)
	{
		ImGui::GetIO().MouseDrawCursor = gui.visible;

		if (!gui.visible)
		{
			// if gui is visible, regular drawing will apply the cursor state,
			// but if not, draw one empty frame to update the state
			ImGui_ImplOpenGL3_NewFrame();
			ImGui_ImplSDL2_NewFrame();
			ImGui::NewFrame();
			ImGui::Render();
			ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
			ImGui::EndFrame();

			return;
		}
	}
	else
	#endif
	if (!gui.visible) return;

	ImGui_ImplOpenGL3_NewFrame();
	#ifdef LINUX
	ImGui_ImplSDL2_NewFrame();
	#else
	ImGui_ImplAndroid_NewFrame();
	#endif
	ImGui::NewFrame();

	gui.draw();

	ImGui::Render();
	ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}


void Renderer::draw()
{
	GL_CHECK;
	#ifdef ANDROID
	if (eglGetCurrentContext() != context) return;
	#endif

	doc.draw();
	window.animate();

	const int N = doc.puzzle.N;
	if (current_N < N || current_N > std::max(N+100, N*3/2)) alloc_puzzle_VOs(true);

	const Camera &camera = doc.camera;
	int w = camera.screen_w(), h = camera.screen_h();

	glViewport(0, 0, w, h);
	Preferences::bg_color().set_clear();
	#ifdef LINUX
	glClearDepth(1.0);
	#else
	glClearDepthf(1.0f);
	#endif
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	GL_CHECK;
	if (camera.empty()) return;

	draw_background();
	GL_CHECK;
	draw_puzzle();
	GL_CHECK;
	draw_buttons();
	GL_CHECK;
	draw_gui();
	GL_CHECK;

	++current_buf; current_buf %= 2;
}
