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

	std::filesystem::path directory();

	EdgeType edge();
	void edge(EdgeType value);

	int  fps();
	void fps(int value);

	float solution_alpha();
	void  solution_alpha(float value);

	bool absolute_mode();
	void absolute_mode(bool value);

	float button_scale();
	void  button_scale(float value);
	ScreenEdge button_edge();
	void  button_edge(ScreenEdge value);
	ScreenAlign button_align();
	void  button_align(ScreenAlign value);

	bool vsync();
	void vsync(bool value);

	bool spiral();
	void spiral(bool value);

	std::string image();
	void image(const std::string &path);

	int  pieces();
	void pieces(int n);
};
