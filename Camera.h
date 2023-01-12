#pragma once
#include "Utility/Matrix.h"
#include "Persistence/Serializer.h"
#include "Coordinates.h"

#define DEGREES (M_PI/180.0)

// Camera coordinates: x to right, y down (y = 0 is above y = 1)
// Puzzle coordinates: dito, but scaled with Puzzle::sx, sy

class Camera : public Serializable
{
public:
	Camera() : R(1.0), range(1.0, 1.0), center(0.0, 0.0), w(1), h(1), hr(1.0) { }

	void save(Serializer &s) const;
	void load(Deserializer &s);

	void viewport(int w, int h); // set viewport size in pixels
	M4d  matrix(int antialias_pass = 0, int num_passes = 1) const;

	int  screen_w() const{ return w; }
	int  screen_h() const{ return h; }
	double aspect() const{ return hr; }
	double pixel_size() const{ return 2.0*range.x / (w-1); } // => (w-1)*pixelsize = 2*xrange (-1 for the half pixels sticking out from the ends)
	bool empty() const { return w <= 1 || h <= 1; }

	inline ScreenCoords to_screen(const CameraCoords &p) const
	{
		return ScreenCoords(((p.x-center.x) / range.x + 1.0) * (w-1) / 2.0,
		                    ((p.y-center.y) / range.y + 1.0) * (h-1) / 2.0);
	}
	inline CameraCoords from_screen(const ScreenCoords &p) const
	{
		return CameraCoords((2.0 * p.x / (w-1) - 1.0) * range.x + center.x,
		                    (2.0 * p.y / (h-1) - 1.0) * range.y + center.y);
	}
	CameraCoords dconvert(const ScreenCoords &d) const
	{
		return CameraCoords(
			2.0 * range.x * d.x / (w-1),
			2.0 * range.y * d.y / (h-1));
	}

	void zoom(double f);
	void zoom(double f, const ScreenCoords &center);
	void move(const CameraCoords &d) { center += d; }

	void view_box(double x0, double x1, double y0, double y1, double factor = 1.0)
	{
		center.set((x0+x1)*0.5, (y0+y1)*0.5);
	#if 0
		// this makes it fit, no matter how the window gets resized:
		R = std::max(x1-x0, y1-y0)*0.5*factor;
		if (w < h) range.set(R, R * hr);
		else       range.set(R / hr, R);
	#else
		// this fits it with the current w,h:
		R = std::max((x1-x0)*h, (y1-y0)*w) * 0.5*factor;
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

	CameraCoords center, range;
	double R; // min range (range is computed from this)

private:
	int    w, h;    // current display port size in pixels
	double hr;      // aspect ratio: h / w
};
