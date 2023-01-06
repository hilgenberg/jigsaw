#include "ImagePuzzle.h"
#include "Utility/Histogram.h"

Histogram &ImagePuzzle::histogram()
{
	if (!histo) histo.reset(new Histogram(im, W, H));
	return *histo;
}

bool ImagePuzzle::load(const std::string &p)
{
	GL_Image im2; if (!im2.load(p)) return false;
	im_path = p;
	im.swap(im2);
	histo = nullptr;
	return true;
}

void ImagePuzzle::load(Deserializer &s)
{
	Puzzle::load(s);
	s.string_(im_path);
	im.load(im_path);
	histo = nullptr;
}
void ImagePuzzle::save(Serializer &s) const
{
	Puzzle::save(s);
	s.string_(im_path);
}
