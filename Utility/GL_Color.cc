#include "GL_Color.h"
#include <cstring>
#include "StringFormatting.h"

void GL_Color::save(Serializer &s) const
{
	s.float_(r);
	s.float_(g);
	s.float_(b);
	s.float_(a);
}
void GL_Color::load(Deserializer &s)
{
	s.float_(r);
	s.float_(g);
	s.float_(b);
	s.float_(a);
}

void GL_Color::set_clear() const
{
	glClearColor(r, g, b, a);
}

std::ostream &operator<<(std::ostream &os, const GL_Color &c)
{
	os << '(' << c.r << ", " << c.g << ", " << c.b << ", " << c.a << ')';
	return os;
}
