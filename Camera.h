#pragma once
#include "Matrix.h"
#include "Persistence/Serializer.h"

#define DEGREES (M_PI/180.0)

class Camera : public Serializable
{
public:
	Camera() : R(1.0), range(1.0, 1.0), center(0.0, 0.0), w(1), h(1), hr(1.0) { }

	void save(Serializer &s) const;
	void load(Deserializer &s);

	void viewport(int w, int h); // set view port size in pixels
	void set(GLuint uniform, int antialias_pass = 0, int num_passes = 1) const;

	int  screen_w() const{ return w; }
	int  screen_h() const{ return h; }
	double aspect() const{ return hr; }
	double pixel_size() const{ return 2.0*range.x / w; } // => w*pixelsize = 2*xrange
	bool empty() const { return w <= 1 || h <= 1; }

	void reset();
	void zoom(double f);
	void move(double dx, double dy);
	void translate(double dx, double dy, double dz, int mx, int my);
	P2f convert(int mx, int my, bool absolute) const;
	void view_box(float x0, float x1, float y0, float y1, float factor = 1.0f)
	{
		center.set((x0+x1)*0.5f, (y0+y1)*0.5f);
		R = std::max((x1-x0)*0.5f, (y1-y0)*0.5f)*factor;
		if (w < h) range.set(R, R * hr);
		else       range.set(R / hr, R);
	}

	P2d    center, range;

private:
	double R; // min range (range is computed from this)
	int    w, h;    // current display port size in pixels
	double hr;      // aspect ratio: h / w
};
