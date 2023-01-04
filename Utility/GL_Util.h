#pragma once
#ifndef DEBUG

#define GL_CHECK

#elif defined(LINUX)

#include <iostream>

#define GL_CHECK do{\
	GLenum err = glGetError();\
	if (err) std::cerr << "glERROR: " << gluErrorString(err) << " at " << __FILE__ << ", " << __LINE__ << std::endl;\
	assert(err == GL_NO_ERROR);\
}while(0)

#elif defined(ANDROID)

extern const char *gl_error_string(GLenum err) noexcept;
#include <android/log.h>

#define GL_CHECK do{\
	GLenum err = glGetError();\
	if (err) __android_log_print(ANDROID_LOG_ERROR, "JIGSAW", "glERROR: %s at %s, %d\n", gl_error_string(err), __FILE__, __LINE__);\
	assert(err == GL_NO_ERROR);\
}while(0)

#else
#error "add GL_CHECK implementation"
#endif
