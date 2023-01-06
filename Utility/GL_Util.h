#pragma once
#ifndef DEBUG

#define GL_CHECK

#elif defined(LINUX)

#define GL_CHECK do{\
	GLenum err = glGetError();\
	if (err) LOG_ERROR("glERROR: %s", gluErrorString(err));\
	assert(err == GL_NO_ERROR); }while(0)

#elif defined(ANDROID)

extern const char *gl_error_string(GLenum err) noexcept;

#define GL_CHECK do{\
	GLenum err = glGetError();\
	if (err) LOG_ERROR("glERROR: %s", gl_error_string(err));\
	assert(err == GL_NO_ERROR); }while(0)

#else
#error "add GL_CHECK implementation"
#endif
