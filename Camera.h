#pragma once
#include "Utility/Matrix.h"
#include "Persistence/Serializer.h"

#define DEGREES (M_PI/180.0)

// Camera coordinates: x to right, y down (y = 0 is above y = 1)
// Puzzle coordinates: dito, but scaled with Puzzle::sx, sy

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
	P2f convert(int mx, int my) const;
	P2f dconvert(double dx, double dy) const;

	void reset();
	void zoom(double f);
	void move(double dx, double dy);
	void translate(double dx, double dy, double dz, int mx = -1, int my = -1);
	void view_box(float x0, float x1, float y0, float y1, float factor = 1.0f)
	{
		center.set((x0+x1)*0.5f, (y0+y1)*0.5f);
	#if 0
		// this makes it fit, no matter how the window gets resized:
		R = std::max(x1-x0, y1-y0)*0.5f*factor;
		if (w < h) range.set(R, R * hr);
		else       range.set(R / hr, R);
	#else
		// this fits it with the current w,h:
		R = std::max((x1-x0)*h, (y1-y0)*w) * 0.5f*factor;
		range.set(R/h, R/w);
		R /= std::max(w, h);
	#endif
	}
	void set_R(double R_)
	{
		R = R_;
		if (w < h) range.set(R, R * hr);
		else       range.set(R / hr, R);
	}

	P2d center, range;
	double R; // min range (range is computed from this)

private:
	int    w, h;    // current display port size in pixels
	double hr;      // aspect ratio: h / w
};
