#ifdef ANDROID
#include "License.h"
#include "Utility/Preferences.h"

bool license()
{
	return true;
	//return Preferences::cached_license();
}

#endif