#pragma once

//  All released versions as major<<16 + minor
#define FILE_VERSION_1_0  0x00010000U
#define CURRENT_VERSION  FILE_VERSION_1_0

inline const char *version_string(unsigned v)
{
	static char buf[32];
	#pragma warning(suppress: 6031) // return of snprintf ignored
	snprintf(buf, 31, "%u.%u", v >> 16, v & 0xFFFF);
	return buf;
}
