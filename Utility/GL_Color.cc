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
