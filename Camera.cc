#include "Camera.h"
#include "OpenGL/GL_Util.h"
#include <GL/gl.h>
#include <GL/glu.h>

void Camera::save(Serializer &s) const
{
	s.double_(center.x);
	s.double_(center.y);
	s.double_(R);
}
void Camera::load(Deserializer &s)
{
	s.double_(center.x);
	s.double_(center.y);
	s.double_(R);
	if (w < h) { range.x = R; range.y = R * hr; }
	else       { range.y = R; range.x = R / hr; }
}

void Camera::viewport(int w_, int h_)
{
	w = w_;
	h = h_;
	hr = (double)h / w;
	if (w < h) { range.x = R; range.y = R * hr; }
	else       { range.y = R; range.x = R / hr; }
}

void Camera::set(GLuint u, int aa_pass, int num_passes) const
{
	static constexpr double jit4[4][2] = {{0.375, 0.25}, {0.125, 0.75}, {0.875, 0.25}, {0.625, 0.75}};
	static constexpr double jit8[8][2] = {{0.5625, 0.4375}, {0.0625, 0.9375}, {0.3125, 0.6875}, {0.6875, 0.8125}, {0.8125, 0.1875}, {0.9375, 0.5625}, {0.4375, 0.0625}, {0.1875, 0.3125}};

	assert(num_passes == 1 || num_passes == 4 || num_passes == 8);
	assert(aa_pass >= 0 && aa_pass < num_passes);
	
	M4d m(1.0);

	double l = center.x - range.x, r = center.x + range.x, 
	       t = center.y - range.y, b = center.y + range.y,
	       n = -1.0, f = 1.0;
	switch (num_passes)
	{
		case 1: break;
		case 4:
		{
			double dx = (l-r)*jit4[aa_pass][0]/w;
			double dy = (b-t)*jit4[aa_pass][1]/h;
			l += dx; r += dx;
			b += dy; t += dy;
			break;
		}
		case 8:
		{
			double dx = (l-r)*jit8[aa_pass][0]/w;
			double dy = (b-t)*jit8[aa_pass][1]/h;
			l += dx; r += dx;
			b += dy; t += dy;
			break;
		}
		default: assert(false);
	}

	m.a11 = 2.0 / (r-l);
	m.a22 = 2.0 / (t-b);
	m.a33 = 2.0 / (n-f);
	m.a14 = (l+r) / (l-r);
	m.a24 = (b+t) / (b-t);
	m.a34 = (n+f) / (n-f);

	/*M4d m(1.0), im(1.0);
	im.a11 = (r-l)*0.5;
	im.a22 = (t-b)*0.5;
	im.a33 = (n-f)*0.5;
	im.a14 = (l+r)*0.5;
	im.a24 = (b+t)*0.5;
	im.a34 = (n+f)*0.5;*/

	uniform(u, m);
}

void Camera::move(double dx, double dy)
{
	center.x += dx;
	center.y += dy;
}

void Camera::zoom(double f)
{
	constexpr double zmin = 1.0e-12, zmax = 1.0e12;
	range *= f;
	if (range.x < zmin)
	{
		range.x = zmin;
		range.y = range.x * hr;
	}
	else if (range.x > zmax)
	{
		range.x = zmax;
		range.y = range.x * hr;
	}
}

void Camera::translate(double dx, double dy, double dz, int mx, int my)
{
	double f = exp(-dz * 0.02);
	double pixel = pixel_size();
	
	dx *= pixel;
	dy *= pixel;
	{
		move(-dx, dy);

		// zoom with dz, but keep (mx,my) where it is
		double x0 = 2.0 * mx / (w-1) - 1.0; // [-1,1]
		double y0 = 2.0 * my / (h-1) - 1.0;
		double x1 = x0 * range.x + center.x;
		double y1 = y0 * range.y + center.y;
		zoom(f);
		if (w > 1 && h > 1 && mx >= 0 && my >= 0 && mx < w && my < h)
		{
			double x2 = x0 * range.x + center.x;
			double y2 = y0 * range.y + center.y;
			move(x1-x2, y1-y2);
		}
	}
}
P2f Camera::convert(int mx, int my, bool absolute) const
{
	if (absolute)
	{
		double x0 = 2.0 * mx / (w-1) - 1.0; // [-1,1]
		double y0 = 2.0 * my / (h-1) - 1.0;
		double x2 = x0 * range.x + center.x;
		double y2 = y0 * range.y + center.y;
		return P2f(x2, y2);
	}

	return P2f(mx * 2.0*range.x / w, my * 2.0*range.y / h);
}
