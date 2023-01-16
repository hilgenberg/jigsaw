#pragma once
#ifdef LINUX
inline constexpr bool license() { return
	//false;
	true;
}
inline void buy_license() {}

#else

bool license(); // is this the full version?
extern void buy_license(); // in Android.cc

#endif