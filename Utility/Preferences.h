#pragma once
#include <filesystem>
#include "GL_Color.h"
struct Document;

/**
 * For getting and setting user preferences.
 * The getters must be threadsafe.
 * Setters should be persistent.
 */

enum EdgeType
{
	None = 0, // plain rectangles
	Regular,  // regular jigsaw pieces
	Triangle, // _/\_ triangular cutout
	Groove,   // --_--
	Circle,   // semicircle
};

enum ScreenEdge
{
	LEFT = 0,
	RIGHT,
	TOP,
	BOTTOM
};

enum ScreenAlign
{
	TOP_OR_LEFT = 0,
	CENTERED,
	BOTTOM_OR_RIGHT
};

namespace Preferences
{
	bool flush(); // store changes into registry/ini file
	bool reset(); // reread from registry/disk
	void reset_to_factory();

	bool save_state(const Document &doc);
	bool load_state(      Document &doc);

	std::filesystem::path directory();
	#ifdef ANDROID
	void directory(const std::string &data_dir);
	#endif

	#define PREFV(type, name) type name(); void name(type value)
	#define PREFR(type, name) type name(); void name(const type &value)
	
	#ifdef LINUX
	PREFR(std::string, image);
	PREFV(int,         pieces);
	#endif
	
	PREFV(EdgeType,    edge);
	PREFV(int,         Nmax);
	PREFR(GL_Color,    bg_color);
	PREFV(float,       solution_alpha);
	PREFV(bool,        absolute_mode);
	PREFV(float,       button_scale);
	PREFV(ScreenEdge,  button_edge);
	PREFV(ScreenAlign, button_align);
	PREFV(bool,        spiral);
	PREFV(float,       finger_radius); // in pixels
	PREFV(bool,        click);
	PREFV(bool,        hide_help);
	#ifdef ANDROID
	PREFV(bool,        vibrate);
	PREFV(bool,        adaptive_touch);
	#endif

	#undef PREFV
	#undef PREFR

	inline bool dark_mode() { return bg_color().lightness() < 0.5f; }
};

#ifdef LINUX
inline void send_email() {}
#else
extern void send_email(); // in Android.cc
#endif
