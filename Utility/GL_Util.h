#pragma once
#include <map>
#include "Vector.h"
#include "Matrix.h"
#include <cassert>
#include <GL/gl.h>

GLuint compileShader(const char *src, GLuint type);
GLuint compileShaders(const std::map<GLuint, const char *> &shaders);

void uniform(GLuint index, const M3d &mat);
void uniform(GLuint index, const M4d &mat);

#ifdef DEBUG
#include <GL/glu.h>
#include <iostream>

#define GL_CHECK do{\
	GLenum err = glGetError();\
	if (err) std::cerr << "glERROR: " << gluErrorString(err) << " at " << __FILE__ << ", " << __LINE__ << std::endl;\
	assert(err == GL_NO_ERROR);\
}while(0)

#else
#define GL_CHECK
#endif
