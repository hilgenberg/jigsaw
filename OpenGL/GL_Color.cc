#include "GL_Color.h"
#include <GL/gl.h>
#include <cstring>
#include "../Utility/StringFormatting.h"

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

void GL_Color::set() const
{
	glColor4fv(v);
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

static double hue2rgb(double p, double q, double t)
{
	if(t < 0.0) t += 6.0;
	if(t > 6.0) t -= 6.0;
	if(t < 1.0) return p + (q - p) * t;
	if(t < 3.0) return q;
	if(t < 4.0) return p + (q - p) * (4.0 - t);
	return p;
}
void GL_Color::hsl(double h, double s, double l)
{
	if (h < 0.0) h = 0.0; else if (h > 1.0) h = 1.0;
	if (s < 0.0) s = 0.0; else if (s > 1.0) s = 1.0;
	if (l < 0.0) l = 0.0; else if (l > 1.0) l = 1.0;
	if (s == 0.0) // achromatic
	{
		r = g = b = (float)l;
	}
	else
	{
		h *= 0.5*M_1_PI; if (h < 0.0) ++h;
		double q = l < 0.5 ? l * (1.0 + s) : l + s - l * s;
		double p = 2.0 * l - q;
		r = (float)hue2rgb(p, q, 6.0*h + 2.0);
		g = (float)hue2rgb(p, q, 6.0*h);
		b = (float)hue2rgb(p, q, 6.0*h - 2.0);
	}
}


const std::map<std::string, GL_Color> GL_Color::named_colors()
{
	static std::map<std::string, GL_Color> C;
	if (C.empty())
	{
		C["red"]   = GL_Color(1.0f, 0.0f, 0.0f);
		C["green"] = GL_Color(0.0f, 1.0f, 0.0f);
		C["blue"]  = GL_Color(0.0f, 0.0f, 1.0f);
		C["white"] = GL_Color(1.0f);
		C["black"] = GL_Color(0.0f);
	}
	return C;
}

static inline unsigned char d2n(char c)
{
	assert(isxdigit(c));
	return c >= '0' && c <= '9' ? c-'0' :
	       c >= 'a' && c <= 'f' ? c-'a'+10 :
	       c-'A'+10;
}
static bool read_hex(const char *s, GL_Color &c)
{
	// [#]RRGGBB[AA] | [#]RGB[A]
	if (*s == '#') ++s;
	int n = 0;
	for (; s[n]; ++n) if (!isxdigit(s[n])) return false;
	if (n < 3 || n > 8 || n == 5 || n == 7) return false;
	unsigned char r, g, b, a;
	
	switch (n)
	{
		case 3:
		case 4:
			r = d2n(*s++)*0x11;
			g = d2n(*s++)*0x11;
			b = d2n(*s++)*0x11;
			a = (n==3 ? 255 : d2n(*s)*0x11);
			break;
		case 6:
		case 8:
			r = d2n(*s)*0x10 + d2n(s[1]); s += 2;
			g = d2n(*s)*0x10 + d2n(s[1]); s += 2;
			b = d2n(*s)*0x10 + d2n(s[1]); s += 2;
			a = (n==6 ? 255 : d2n(*s)*0x10 + d2n(s[1]));
			break;

		default: return false;
	}

	c.r = (float)r / 255.0f;
	c.g = (float)g / 255.0f;
	c.b = (float)b / 255.0f;
	c.a = (float)a / 255.0f;
	return true;
}
