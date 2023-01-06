#pragma once
#include "Matrix.h"

class GL_Program
{
public:
	GL_Program() {}
	void add_uniform(const char *type, const char *name); // call this only before add_shaders (otherwise debug build will complain)!
	void add_shaders(const char *vertex, const char *geometry, const char *fragment); // pass NULL to use same shader as variant #0
	~GL_Program();

	void use(int variant = 0);
	void finish();
	#ifdef ANDROID
	void drop() { assert(!active); program = 0; }
	#endif

	void uniform(int i, int   value);
	void uniform(int i, float value);
	void uniform(int i, int    v1, int    v2);
	void uniform(int i, float  v1, float  v2);
	void uniform(int i, double v1, double v2);
	void uniform(int i, const M3d &mat);
	void uniform(int i, const M4d &mat);

private:
	std::vector<std::string> V, G, F;
	GLuint program = 0;
	bool active = false;
	int  compiled_variant = -1;
	std::vector<std::string> uniform_names, uniform_types;

	#ifdef ANDROID
	std::vector<int> uniform_locations;
	constexpr static const char *glsl_version = "#version 320 es";
	#else
	constexpr static const char *glsl_version = "#version 430 core";
	#endif
};