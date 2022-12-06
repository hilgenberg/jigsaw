class Histogram
{
public:
	typedef int Piece;
	Histogram(const GL_Image &im, int W, int H);
	double distance(Piece i, Piece j) const; // [0,1]
private:
	std::vector<std::array<unsigned,64>> hist;
	std::vector<unsigned> total;
};

Histogram::Histogram(const GL_Image &im, int W, int H)
{
	std::cout << "analyzing image..." << std::endl;
	hist.resize(W*H); for (auto &h : hist) h.fill(0);
	total.resize(W*H, 0);
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
