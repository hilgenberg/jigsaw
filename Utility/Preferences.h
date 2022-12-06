#pragma once
#include <filesystem>

/**
 * For getting and setting user preferences.
 * The getters must be threadsafe.
 * Setters should be persistent.
 */

namespace Preferences
{
	bool flush(); // store changes into registry/ini file
	bool reset(); // reread from registry/disk

	std::filesystem::path directory();

	bool showFPS();
	void showFPS(bool value);

	int  fps();
	void fps(int value);

	bool vsync();
	void vsync(bool value);

	std::string image();
	void image(const std::string &path);

	int pieces();
	void pieces(int n);
};
