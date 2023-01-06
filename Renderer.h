#pragma once
#include "Utility/GL_Program.h"
struct Document;
class Window;

class Renderer
{
public:
	Renderer(Document &doc);
	~Renderer();
	void draw();
	void redraw() { want_redraw = true; }
	bool wants_redraw() const { return want_redraw; }

private:
	Document &doc;
	bool want_redraw = true;
	
	#ifdef ANDROID
	const EGLContext context; // which GL context do all our objects live in?
	#endif

	void draw_background();
	void draw_puzzle();
	void draw_buttons();

	int current_buf = 0; // we use double buffering for data to avoid stalls in glMap

	GL_Program program;
	GLuint VBO[2] = {0,0}, VAO[2] = {0,0};
	GLuint texture = 0;

	GL_Program bg_program;
	GLuint bg_VAO = 0;

	GL_Program button_program;
	GLuint button_VBO[2] = {0,0}, button_VAO[2] = {0,0};
	GLuint button_texture = 0;

};
