#include "Serializer.h"
#include <cassert>
#include <iostream>
#include <cstring>
#include <memory>

//----------------------------------------------------------------------------------------------------------------------
//  Serializer
//----------------------------------------------------------------------------------------------------------------------

Serializer::Serializer(ByteWriter &f) : ver(CURRENT_VERSION), f(f)
{
	marker_("PUZZLE");
	uint32_(ver);
}

void Serializer::bool_(bool x){ char c = x; f.write(&c, 1); }
void Serializer::uint16_(uint16_t x){ f.write(&x, 2); }
void Serializer::int32_ (int32_t  x){ f.write(&x, 4); }
void Serializer::uint32_(uint32_t x){ f.write(&x, 4); }
void Serializer::int64_ (int64_t  x){ f.write(&x, 8); }
void Serializer::uint64_(uint64_t x){ f.write(&x, 8); }

void Serializer::enum_(int x, int min, int max)
{
	if (x < min || x > max)
		throw std::logic_error("enum out of range");
	int32_(x);
}

void Serializer::float_ (float  x){ f.write(&x, 4); }
void Serializer::double_(double x){ f.write(&x, 8); }

void Serializer::raw_(std::vector<unsigned char> &data)
{
	if (!data.empty()) f.write(data.data(), data.size());
}

void Serializer::string_(const std::string &x)
{
	size_t len = x.length();
	uint64_(len);
	if (len) f.write(x.data(), len);
}

void Serializer::marker_(const char *s)
{
	f.write(s, strlen(s));
}

//----------------------------------------------------------------------------------------------------------------------
//  Deserializer
//----------------------------------------------------------------------------------------------------------------------

Deserializer::Deserializer(ByteReader &f) : f(f)
{
	marker_("PUZZLE");
	uint32_(ver);
	if (ver < FILE_VERSION_1_0)
	{
		throw std::runtime_error("Invalid version info read. Save game seems corrupted.");
	}
	if (ver > CURRENT_VERSION)
	{
		std::ostringstream s;
		s << "This file was written by a newer version. It requires at least version " << version_string(ver) << " to read it.";
		throw std::runtime_error(s.str());
	}
}

void Deserializer::bool_(bool &x){ char c; f.read(&c, 1); x = c; }
void Deserializer::uint16_(uint16_t &x){ f.read(&x, 2); }
void Deserializer::int32_ (int32_t  &x){ f.read(&x, 4); }
void Deserializer::uint32_(uint32_t &x){ f.read(&x, 4); }
void Deserializer::int64_ (int64_t  &x){ f.read(&x, 8); }
void Deserializer::uint64_(uint64_t &x){ f.read(&x, 8); }

void Deserializer::float_(float   &x){ f.read(&x, 4); }
void Deserializer::double_(double &x){ f.read(&x, 8); }

void Deserializer::raw_(std::vector<unsigned char> &data, size_t n)
{
	data.resize(n);
	if (n) f.read(data.data(), n);
}

void Deserializer::marker_(const char *s_)
{
	const char *s = s_;
	size_t N = strlen(s);
	char buf[16];
	while(N)
	{
		size_t n = std::min(N, (size_t)16);
		f.read(buf, n);
		if (memcmp(s, buf, n) != 0)
		{
			std::ostringstream ss;
			ss << "Marker \"" << s_ << "\" not found. This file seems to be corrupted.";
			throw std::runtime_error(ss.str());
		}
		N -= n;
		s += n;
	}
}

void Deserializer::string_(std::string &x)
{
	uint64_t len; uint64_(len);
	if (len)
	{
		std::unique_ptr<char[]> buf(new char [(size_t)len]);
		f.read(buf.get(), (size_t)len);
		x.assign(buf.get(), (size_t)len);
	}
	else
	{
		x.clear();
	}
}
