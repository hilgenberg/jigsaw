#pragma once
#include "../Persistence/Serializer.h"
#include "GL_Color.h"
#include <vector>

struct GL_Image : public Serializable
{
	GL_Image() : _w(0), _h(0), _opacity(1), _state(0){ }
	GL_Image(const GL_Image &i)
	: _state(0)
	, _w(i._w), _h(i._h), _data(i._data), _opacity(i._opacity)
	{
		check_data();
	}
	
	#ifdef __linux__
	bool load(const std::string &path);
	#endif
	
	GL_Image &operator=(const GL_Image &x)
	{
		_w       = x._w;
		_h       = x._h;
		_data    = x._data;
		_opacity = x._opacity;
		check_data();
		++_state;
		return *this;
	}

	GL_Image &swap(GL_Image &x)
	{
		std::swap(_w, x._w);
		std::swap(_h, x._h);
		std::swap(_data, x._data);
		std::swap(_opacity, x._opacity);
		++_state; ++x._state;
		return *this;
	}

	bool operator==(const GL_Image &x) const
	{
		return _w == x._w && _h == x._h && _data == x._data;
	}

	inline void clear() { redim(0, 0); ++_state; }
	
	virtual void save(Serializer   &s) const;
	virtual void load(Deserializer &s);
	
	unsigned char *redim(unsigned w, unsigned h)
	{
		_w = w; _h = h;
		_data.resize(_w * _h * 4);
		_opacity = -1;
		++_state;
		return _data.data();
	}
	
	const std::vector<unsigned char> &data() const;

	unsigned w() const{ return _w; }
	unsigned h() const{ return _h; }
	bool empty() const{ return _w == 0 || _h == 0; }
	size_t state_counter() const { return _state; }
	
	bool opaque() const
	{
		if (_opacity < 0)
		{
			_opacity = 1;
			const unsigned char *d = data().data();
			for (size_t i = 3, n = data().size(); i < n; i += 4)
			{
				if ((unsigned char)(d[i]+1) > 1)
				{
					_opacity = 0; break;
				}
			}
		}
		return _opacity > 0;
	}
	
	void prettify(bool circle=false);
	
	void mix(const GL_Color &base, float alpha, std::vector<unsigned char> &dst) const;
	void mix(float alpha, std::vector<unsigned char> &dst) const;
	
private:
	unsigned                   _w, _h;   // width and height
	std::vector<unsigned char> _data;    // rgba, size = 4*w*h
	mutable short              _opacity; // -1 = unknown, 0 = transparent, 1 = opaque
	size_t                     _state;   // incremented on every modification

	inline void check_data() const
	{
		assert(_data.size() == (size_t)_w * _h * 4);
	}
};
