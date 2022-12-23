#pragma once
#include <filesystem>

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

namespace Preferences
{
	bool flush(); // store changes into registry/ini file
	bool reset(); // reread from registry/disk

	std::filesystem::path directory();

	EdgeType edge();
	void edge(EdgeType value);

	int  fps();
	void fps(int value);

	float solution_alpha();
	void solution_alpha(float value);

	bool absolute_mode();
	void absolute_mode(bool value);

	bool vsync();
	void vsync(bool value);

	std::string image();
	void image(const std::string &path);

	int pieces();
	void pieces(int n);
};
