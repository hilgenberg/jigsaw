#include <map>
#include "../Vector.h"
#include "../Matrix.h"

GLuint compileShader(const char *src, GLuint type);
GLuint compileShaders(const std::map<GLuint, const char *> &shaders);

void uniform(GLuint index, const M3d &mat);
void uniform(GLuint index, const M4d &mat);
