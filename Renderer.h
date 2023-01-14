#pragma once
#include "Utility/GL_Program.h"
#ifdef LINUX
#include <SDL.h>
#endif
#ifdef ANDROID
struct ANativeWindow;
#endif
struct Document;
class Window;
class GUI;

class Renderer
{
public:
	Renderer(Document &doc, Window &window, GUI &gui, 
	#ifdef LINUX
		SDL_Window *sdl_window, SDL_GLContext context
	#else
		ANativeWindow *jni_window
	#endif
	);

	~Renderer();
	void draw();

private:
	Document &doc;
	Window   &window;
	GUI      &gui;
	
	#ifdef ANDROID
	const EGLContext context; // which GL context do all our objects live in?
	ANativeWindow *jni_window = NULL; // only to call ANativeWindow_release on it
	#endif
	
	void alloc_puzzle_VOs(bool free_old_buffers); // realloc when puzzle.N changes

	void draw_background();
	void draw_puzzle();
	void draw_buttons();
	void draw_gui();

	int current_buf = 0; // we use double buffering for data to avoid stalls in glMap
	int current_N = 0; // from last alloc_puzzle_VOs call

	GL_Program program;
	GLuint VBO[2] = {0,0}, VAO[2] = {0,0};
	GLuint texture = 0;

	GL_Program bg_program;
	GLuint bg_VAO = 0;

	GL_Program button_program;
	GLuint button_VBO[2] = {0,0}, button_VAO[2] = {0,0};
	GLuint button_texture = 0;
};
