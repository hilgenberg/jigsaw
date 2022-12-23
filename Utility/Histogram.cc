#include "Histogram.h"

Histogram::Histogram(const GL_Image &im, int W, int H)
{
	std::cout << "analyzing image..." << std::endl;
	hist.resize(W*H); for (auto &h : hist) h.fill(0);
	total.resize(W*H, 0); unit.resize(W*H, 0.0);
	const unsigned char *data = im.data().data();
	for (int y = 0, h = im.h(); y < h; ++y)
	{
		int j = y * H / h;
		for (int x = 0, w = im.w(); x < w; ++x, data += 4)
		{
			int i = x * W / w, k = j*W+i;
			int r = (int)(data[0]/64), g = (int)(data[1]/64), b = (int)(data[2]/64);
			assert(r >= 0 && r < 4);
			assert(g >= 0 && g < 4);
			assert(b >= 0 && b < 4);
			int bin = (r<<4) + (g<<2) + b;
			//std::cout << "bin " << bin << ", " << r << ", " << g << ", " << b << std::endl;
			assert(bin >= 0 && bin < 64);
			++hist[k][bin];
			++total[k];
		}
	}
	for (int j = 0; j < H; ++j)
	{
		for (int i = 0; i < W; ++i)
		{
			auto &h = hist[j*W+i];
			int sum = 0;
			for (int k = 0; k < 64; ++k)
			{
				sum += h[k]*h[k];
			}
			unit[j*W+i] = sqrt(sum);
		}
	}
	std::cout << "...done" << std::endl;
}

double Histogram::distance(Piece i, Piece j) const
{
	assert(i >= 0 && i < (int)hist.size());
	assert(j >= 0 && j < (int)hist.size());
	double s = 0.0;
	for (int bin = 0; bin < 64; ++bin)
	{
		double a = hist[i][bin], b = hist[j][bin];
		s += fabs(a/total[i]-b/total[j]);
	}
	if (s < 0.0 || s > 2.001)
	{
		std::cout << "S " << s << std::endl;
	}
	assert(s >= 0.0 && s <= 2.001);
	return s*0.5;
}

double Histogram::similarity(Piece i, Piece j) const
{
	assert(i >= 0 && i < (int)hist.size());
	assert(j >= 0 && j < (int)hist.size());
	double s = 0.0;
	for (int bin = 0; bin < 64; ++bin)
	{
		double a = hist[i][bin], b = hist[j][bin];
		s += a*b;
	}
	s /= unit[i]*unit[j];
	if (s < 0.0 || s > 1.001)
	{
		std::cout << "S " << s << std::endl;
	}
	assert(s >= 0.0 && s <= 1.001);
	return s;
}
