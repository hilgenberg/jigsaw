#include "GL_Util.h"
#include <vector>
#include <cstddef>
#include <iostream>

GLuint compileShader(const char *src, GLuint type)
{
	GLuint shader = glCreateShader(type);
	glShaderSource(shader, 1, &src, NULL);
	glCompileShader(shader);
	int success; glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
	if (!success)
	{
		char info[1024];
		glGetShaderInfoLog(shader, 1024, NULL, info);
		std::cerr << "ERROR: Shader compilation failed!\n" << info << std::endl;
		throw std::runtime_error(info);
	}
	return shader;
}

GLuint compileShaders(const std::map<GLuint, const char *> &shaders)
{
	std::vector<GLuint> S;
	try
	{
		for (auto it : shaders)
		{
			auto s = compileShader(it.second, it.first);
			S.push_back(s);
		}
	}
	catch (...)
	{
		for (GLuint s : S) glDeleteShader(s);
		throw;
	}

	GLuint program = glCreateProgram();
	for (GLuint s : S) glAttachShader(program, s);
	glLinkProgram(program);
	int success; glGetProgramiv(program, GL_LINK_STATUS, &success);
	for (GLuint s : S) glDeleteShader(s);
	if (!success)
	{
		char info[1024];
		glGetProgramInfoLog(program, 1024, NULL, info);
		std::cerr << "ERROR: Shader linking failed!\n" << info << std::endl;
		throw std::runtime_error(info);
	}
	return program;
}

void uniform(GLuint index, const M3d &mat)
{
	glUniformMatrix3fv(index, 1, GL_TRUE, (float*)(M3f)mat);
}
void uniform(GLuint index, const M4d &mat)
{
	glUniformMatrix4fv(index, 1, GL_TRUE, (float*)(M4f)mat);
}
