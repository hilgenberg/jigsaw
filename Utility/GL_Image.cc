#include "GL_Image.h"
#include <cassert>
#include <random>
#include <map>
#include <iostream>
#include <cstring>
#include "GL_Util.h"
#include "../data.h"

//----------------------------------------------------------------------------------------------------------------------
// GL_Image
//----------------------------------------------------------------------------------------------------------------------

#define STB_IMAGE_IMPLEMENTATION
#include "../stb/stb_image.h"

bool GL_Image::load(const std::string &path)
{
	if (path == "///sample-data")
		return load(sample_data, sample_data_len);

	//stbi_set_flip_vertically_on_load(true);
	int x,y,n;
	unsigned char *d1 = stbi_load(path.c_str(), &x, &y, &n, 4);
	if (!d1)
	{
		LOG_ERROR("Error loading %s: %s", path.c_str(), stbi_failure_reason());
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
bool GL_Image::load(const unsigned char *data, size_t len)
{
	int x,y,n;
	unsigned char *d1 = stbi_load_from_memory(data, (int)len, &x, &y, &n, 4);
	if (!d1)
	{
		fprintf(stderr, "Error parsing image data: %s\n", stbi_failure_reason());
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
