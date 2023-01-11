#include "GL_Program.h"
#include "Vector.h"
#include "GL_Util.h"
#include "GL_Color.h"

GL_Program::~GL_Program()
{
	if (program) glDeleteProgram(program);
}

void GL_Program::add_uniform(const char *type, const char *name)
{
	assert(!active);
	assert(compiled_variant < 0);
	assert(V.empty() && F.empty() && G.empty());
	uniform_names.emplace_back(name);
	uniform_types.emplace_back(type);
}

void GL_Program::add_shaders(const char *vertex, const char *geometry, const char *fragment)
{
	assert(!active);
	assert(vertex || geometry || fragment);
	assert(vertex   || V.size() == 1);
	assert(fragment || F.size() == 1);
	assert(geometry || G.size() <= 1);
	if (vertex)   V.emplace_back(vertex);
	if (geometry) G.emplace_back(geometry);
	if (fragment) F.emplace_back(fragment);

	#ifdef DEBUG
	int i = std::max(std::max(V.size(), F.size()), G.size())-1;
	use(i); GL_CHECK;
	finish(); GL_CHECK;
	#endif
}

static GLuint compileShader(const std::string &src, GLuint type)
{
	#ifdef DEBUG
	//LOG_DEBUG("Compiling shader %d:\n----------------------------------------\n%s", type, src.c_str());
	#endif

	GLuint shader = glCreateShader(type);
	const char *s = src.c_str();
	glShaderSource(shader, 1, &s, NULL);
	glCompileShader(shader);
	int success; glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
	if (!success)
	{
		char info[1024];
		glGetShaderInfoLog(shader, 1024, NULL, info);
		std::cerr << "ERROR: Shader compilation failed!\n" << info << std::endl;
		glDeleteShader(shader);
		throw std::runtime_error(info);
	}
	return shader;
}

void GL_Program::use(int i)
{
	assert(!active);
	assert(i >= 0);
	assert(V.size() <= 1 || i < (int)V.size());
	assert(G.size() <= 1 || i < (int)G.size());
	assert(F.size() <= 1 || i < (int)F.size());

	if (compiled_variant != i)
	{
		if (program) { glDeleteProgram(program); program = 0; }
		compiled_variant = -1;

		std::string preamble(glsl_version);
		preamble += "\n";
		#ifdef ANDROID
		preamble += "precision mediump float;\n";
		#endif

		for (int i = 0; i < (int)uniform_names.size(); ++i)
		{
			preamble += format(
			#ifdef LINUX
			"layout(location = %d) uniform %s %s;\n", i
			#else
			"uniform %s %s;\n"
			#endif
			, uniform_types[i].c_str(), uniform_names[i].c_str());
		}

		std::vector<GLuint> S;
		try
		{
			if (!V.empty())
			{
				int j = (i < V.size() ? i : 0);
				S.push_back(compileShader(preamble + V[j], GL_VERTEX_SHADER));
			}
			if (!G.empty())
			{
				int j = (i < G.size() ? i : 0);
				S.push_back(compileShader(preamble + G[j], GL_GEOMETRY_SHADER));
			}
			if (!F.empty())
			{
				int j = (i < F.size() ? i : 0);
				S.push_back(compileShader(preamble + F[j], GL_FRAGMENT_SHADER));
			}
		}
		catch (...)
		{
			for (GLuint s : S) glDeleteShader(s);
			throw;
		}

		program = glCreateProgram();
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

		compiled_variant = i;

		#ifdef ANDROID
		uniform_locations.clear();
		for (int i = 0; i < (int)uniform_names.size(); ++i)
		{
			uniform_locations.push_back(glGetUniformLocation(program, uniform_names[i].c_str()));
		}
		#endif
	}

	glUseProgram(program);
	active = true;
}

void GL_Program::finish()
{
	assert(program && active);
	glUseProgram(0);
	active = false;
}

#ifdef ANDROID
#define INDEX_TX do{\
	assert(i >= 0 && i < (int)uniform_names.size());\
	i = uniform_locations[i]; }while (0)
#else
#define INDEX_TX do{\
	assert(i >= 0 && i < (int)uniform_names.size()); }while (0)
#endif


void GL_Program::uniform(int i, float value)
{
	INDEX_TX;
	glUniform1f(i, value);
}
void GL_Program::uniform(int i, int value)
{
	INDEX_TX;
	glUniform1i(i, value);
}
void GL_Program::uniform(int i, float v1, float v2)
{
	INDEX_TX;
	glUniform2f(i, v1, v2);
}
void GL_Program::uniform(int i, int   v1, int   v2)
{
	INDEX_TX;
	glUniform2i(i, v1, v2);
}
void GL_Program::uniform(int i, const M3d &mat)
{
	INDEX_TX;
	glUniformMatrix3fv(i, 1, GL_TRUE, (float*)(M3f)mat);
}
void GL_Program::uniform(int i, const M4d &mat)
{
	INDEX_TX;
	glUniformMatrix4fv(i, 1, GL_TRUE, (float*)(M4f)mat);
}
void GL_Program::uniform(int i, double v1, double v2)
{
	INDEX_TX;
	glUniform2f(i, (float)v1, (float)v2);
}
void GL_Program::uniform(int i, const GL_Color &c)
{
	INDEX_TX;
	glUniform4fv(i, 1, c.v);
}
