#pragma once

#include "ByteReader.h"
#include "FileVersions.h"
#include "ErrorHandling.h"
#include <cstdint>
#include <string>
#include <vector>
#include <map>

/**
 * @addtogroup Serialization
 * @{
 */

class Serializer;
class Deserializer;

/**
 * Base for all classes that are written to files, copy&pasted or saved for undo
 */

class Serializable
{
public:
	virtual void save(Serializer   &s) const = 0;
	virtual void load(Deserializer &s) = 0;
};

/**
 * Frontend for ByteWriter
 */

class Serializer
{
public:
	Serializer(ByteWriter &f);
	
	unsigned version() const{ return ver; }
	
	void bool_(bool x);
	void uint16_(uint16_t x);
	void int32_(int32_t x);
	void uint32_(uint32_t x);
	void int64_(int64_t x);
	void uint64_(uint64_t x);
	void enum_(int x, int min, int max);
	
	void float_(float x);
	void double_(double x);
	
	void string_(const std::string &x);
	void member_(const Serializable &x) { x.save(*this); }
	void marker_(const char *s);
	void raw_(std::vector<unsigned char> &data);
	
private:
	const unsigned ver; ///< (major << 16) + minor
	ByteWriter &f;
};

/**
 * Frontend for ByteReader
 */

class Deserializer
{
public:
	Deserializer(ByteReader &f);
	
	unsigned version() const{ return ver; }
	bool done() const{ return f.pos() == f.size(); }
	size_t remaining() const{ return (f.size() >= f.pos() ? f.size() - f.pos() : 0); }
	
	void bool_(bool &x);
	void uint16_(uint16_t &x);
	void int32_(int32_t &x);
	void uint32_(uint32_t &x);
	void int64_(int64_t &x);
	void uint64_(uint64_t &x);
	//void enum_(int &x, int min, int max);
	template<typename T> void enum_(T &x, T min, T max)
	{
		int32_t t;
		int32_(t);
		if (t < min || t > max)
			throw std::runtime_error("enum out of range");
		x = (T)t;
	}
	void float_(float &x);
	void double_(double &x);

	void string_(std::string &x);
	void member_(Serializable  &x) { x.load(*this); }
	void marker_(const char *s);
	void raw_(std::vector<unsigned char> &data, size_t n);

private:
	ByteReader &f;
	unsigned  ver;
};

/** @} */

