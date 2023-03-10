#pragma once
#include "../Persistence/Serializer.h"
#include <string>
#include <map>

struct GL_Color : public Serializable
{
	GL_Color(float r, float g, float b, float a = 1.0f) : r(r), g(g), b(b), a(a){ }
	GL_Color(float y = 1.0f, float a = 1.0f) : r(y), g(y), b(y), a(a){ }
	GL_Color(const GL_Color &) = default;

	virtual void save(Serializer   &s) const;
	virtual void load(Deserializer &s);
	
	bool     visible() const{ return a >= 1e-8f; }
	bool      opaque() const{ return a >= 1.0f-1e-8f; }
	float  lightness() const{ return 0.299f*r + 0.587f*g + 0.114f*b; }
	void   light_mul(float f)
	{
		if (f <= 1.0f)
		{
			r *= f;
			g *= f;
			b *= f;
		}
		else
		{
			r = (f - 1.0f + r) / f;
			g = (f - 1.0f + g) / f;
			b = (f - 1.0f + b) / f;
		}
	}

	union
	{
		struct{ float r, g, b, a; };
		float v[4];
	};
	
	void clamp()
	{
		if (r < 0.0f) r = 0.0f; if (r > 1.0f) r = 1.0f;
		if (g < 0.0f) g = 0.0f; if (g > 1.0f) g = 1.0f;
		if (b < 0.0f) b = 0.0f; if (b > 1.0f) b = 1.0f;
		if (a < 0.0f) a = 0.0f; if (a > 1.0f) a = 1.0f;
	}
	
	void set_clear() const;
	
	bool operator== (const GL_Color &c) const{ return fabsf(r-c.r) < 1e-8f && fabsf(g-c.g) < 1e-8f && fabsf(b-c.b) < 1e-8f && fabsf(a-c.a) < 1e-8f; }
	bool operator!= (const GL_Color &c) const{ return !operator==(c); }
};
