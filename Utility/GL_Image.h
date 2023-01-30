#pragma once
#include "GL_Color.h"
#include <vector>

struct GL_Image final
{
	GL_Image() : _w(0), _h(0) { }
	GL_Image(const GL_Image &i)
	: _w(i._w), _h(i._h), _data(i._data)
	{
		check_data();
	}
	
	bool load(const std::string &path);
	bool load(const unsigned char *data, size_t len);
	
	GL_Image &operator=(const GL_Image &x)
	{
		_w       = x._w;
		_h       = x._h;
		_data    = x._data;
		check_data();
		return *this;
	}

	GL_Image &swap(GL_Image &x)
	{
		std::swap(_w, x._w);
		std::swap(_h, x._h);
		std::swap(_data, x._data);
		return *this;
	}

	bool operator==(const GL_Image &x) const
	{
		return _w == x._w && _h == x._h && _data == x._data;
	}

	inline void clear() { redim(0, 0); }
	
	unsigned char *redim(unsigned w, unsigned h)
	{
		try{ _data.resize(_w * _h * 4); } catch (...) { return NULL; }
		_w = w; _h = h;
		return _data.data();
	}
	
	const std::vector<unsigned char> &data() const;

	unsigned w() const{ return _w; }
	unsigned h() const{ return _h; }
	bool empty() const{ return _w == 0 || _h == 0; }
	
	void mix(const GL_Color &base, float alpha, std::vector<unsigned char> &dst) const;
	void mix(float alpha, std::vector<unsigned char> &dst) const;
	
private:
	unsigned                   _w, _h;   // width and height
	std::vector<unsigned char> _data;    // rgba, size = 4*w*h

	inline void check_data() const
	{
		assert(_data.size() == (size_t)_w * _h * 4);
	}
};
