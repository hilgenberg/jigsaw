#include "GL_Image.h"
#include <cassert>
#include <random>
#include <map>
#include <iostream>
#include <GL/gl.h>
#include <cstring>

//----------------------------------------------------------------------------------------------------------------------
// GL_Image
//----------------------------------------------------------------------------------------------------------------------

#ifdef __linux__
#define STB_IMAGE_IMPLEMENTATION
#include "../stb/stb_image.h"

bool GL_Image::load(const std::string &path)
{
	//stbi_set_flip_vertically_on_load(true);

	int x,y,n;
	unsigned char *d1 = stbi_load(path.c_str(), &x, &y, &n, 4);
	if (!d1)
	{
		fprintf(stderr, "Error loading %s: %s\n", path.c_str(), stbi_failure_reason());
		return false;
	}
	if (x <= 0 || y <= 0 || n <= 0)
	{
		stbi_image_free(d1);
		return false;
	}

	unsigned char *d2 = redim(x, y);
	if (!d2) return false;
	memcpy(d2, d1, x*y*4);
	stbi_image_free(d1);
	return true;
}
#endif

void GL_Image::save(Serializer &s) const
{
	check_data();
	s.uint32_(_w);
	s.uint32_(_h);

	size_t n = (size_t)_w * _h;
	std::vector<unsigned char> channel(4*n);
	unsigned char *cd = channel.data();

	const unsigned char *d = _data.data() + 3;
	for (size_t j = 0; j < n; ++j, d += 4) *cd++ = *d;
	for (int i = 2; i >= 0; --i)
	{
		d = _data.data() + i;
		for (size_t j = 0; j < n; ++j, d += 4) *cd++ = *d - d[1];
	}
	s.data_(channel);
}
void GL_Image::load(Deserializer &s)
{
	s.uint32_(_w);
	s.uint32_(_h);
	
	size_t n = (size_t)_w * _h;
	_data.resize(n * 4);
	std::vector<unsigned char> channel;
	s.data_(channel);
	if (channel.size() != 4*n)
	{
		_w = _h = 0;
		_data.clear();
		throw std::runtime_error("Reading texture data failed. This file seems to be corrupted.");
	}
	const unsigned char *cd = channel.data();
	unsigned char *d = _data.data() + 3;
	for (size_t j = 0; j < n; ++j, d += 4) *d = *cd++;
	for (int i = 2; i >= 0; --i)
	{
		d = _data.data() + i;
		for (size_t j = 0; j < n; ++j, d += 4) *d = *cd++ + d[1];
	}
	
	_opacity = -1;
	check_data();

	++_state;
}

const std::vector<unsigned char> &GL_Image::data() const
{
	return _data;
}

void GL_Image::mix(const GL_Color &base, float alpha, std::vector<unsigned char> &dst) const
{
	check_data();
	const std::vector<unsigned char> &d = data();
	dst.resize(d.size());
	
	unsigned br = (base.r <= 0.0f ? 0 : base.r >= 1.0f ? 255 : (unsigned char)(255.0f * base.r));
	unsigned bg = (base.g <= 0.0f ? 0 : base.g >= 1.0f ? 255 : (unsigned char)(255.0f * base.g));
	unsigned bb = (base.b <= 0.0f ? 0 : base.b >= 1.0f ? 255 : (unsigned char)(255.0f * base.b));
	unsigned ba = (base.a <= 0.0f ? 0 : base.a >= 1.0f ? 255 : (unsigned char)(255.0f * base.a));
	unsigned a1 = (alpha  <= 0.0f ? 0 : alpha  >= 1.0f ? 255 : (unsigned char)(255.0f * alpha ));

	for (size_t i = 0, n = d.size(); i < n; i += 4)
	{
		unsigned a = (a1*d[i+3])/255;
		dst[i  ] = (unsigned char)((d[i  ]*a + br*(255-a))/255);
		dst[i+1] = (unsigned char)((d[i+1]*a + bg*(255-a))/255);
		dst[i+2] = (unsigned char)((d[i+2]*a + bb*(255-a))/255);
		dst[i+3] = (unsigned char)ba;
	}
}
void GL_Image::mix(float alpha, std::vector<unsigned char> &dst) const
{
	check_data();
	const std::vector<unsigned char> &d = data();
	dst = d;

	if (alpha < 1.0-1e-5)
	{
		unsigned a1 = (alpha <= 0.0f ? 0 : alpha >= 1.0f ? 255 : (unsigned char)(255.0f*alpha));
		for (size_t i = 3; i < d.size(); i += 4)
		{
			dst[i] = (unsigned char)(a1 * dst[i] / 255);
		}
	}
}

void GL_Image::prettify(bool circle)
{
	check_data();
	unsigned char *d = _data.data();
	
	size_t n = (size_t)_w * _h, l = data().size(), no = 0, nt = 0;
	for (size_t i = 3; i < l; i += 4){ if (d[i] > 245) ++no; if (d[i] < 10) ++nt; }
	
	if (no == 0 && nt == n) // fully transparent -> make fully opaque
	{
		for (size_t i = 3; i < l; i += 4) d[i] = 255;
		no = n; nt = 0;
	}
	if (circle && nt == 0 && no == n) // fully opaque
	{
		d += 3;
		int r = std::min(_w, _h);
		int x0 = (int)_w - r/2, y0 = (int)_h - r/2;
		for (int i = 0; i < (int)_h; ++i)
		for (int j = 0; j < (int)_w; ++j)
		{
			double rr = hypot((j - x0), (i - y0)) - r*0.4;
			if (rr >= 0.0) *d = std::min((unsigned char)std::max(0.0, 255.0*(1.0 - rr/0.1)), (unsigned char)255);
			d += 4;
		}
	}
}
