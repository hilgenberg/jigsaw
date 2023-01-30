#include "GUI.h"
#include "Utility/Preferences.h"

#ifdef ANDROID
#include "Utility/Timer.h"
#include "Window.h"
extern Window *window;

bool GUI::activate_secret_menu(int ds, int n, int *id, float *x, float *y)
{
	if (n != 4) return false;
	#ifndef DEBUG
	if (!Preferences::cached_license()) return false; // ignore the ugly flag
	#endif
	const int w = doc.camera.screen_w(), h = doc.camera.screen_h();
	const int s = std::min(w/4, h/4); if (s < 10) return false;
	int i;
	i = -1; while (++i < 4) { if (x[i] <  s  && y[i] <  s ) break; } if (i >= 4) return false;
	i = -1; while (++i < 4) { if (x[i] <  s  && y[i] > h-s) break; } if (i >= 4) return false;
	i = -1; while (++i < 4) { if (x[i] > w-s && y[i] <  s ) break; } if (i >= 4) return false;
	i = -1; while (++i < 4) { if (x[i] > w-s && y[i] > h-s) break; } if (i >= 4) return false;
	show(SECRET_MENU);
	return true;
}

extern bool set_image(const std::string &path);

#ifdef DEBUG
extern void reload_license();
extern bool ugly; extern double ugly_start_time;
#endif
#endif

void GUI::p_secret()
{
	ImVec2 min = ImGui::CalcTextSize("Testing Testing Testing Testing");

	#ifdef ANDROID
	if (ImGui::Button("Load Sample Image", ImVec2(ImGui::GetContentRegionAvail().x,0))) {
		#ifndef NDEBUG
		bool ok =
		#endif
		set_image("///sample-data");
		assert(ok);
		close();
	}
	if (window && ImGui::Button("Play Victory Animation", ImVec2(ImGui::GetContentRegionAvail().x,0)))
	{
		window->play_victory_animation();
		close();
	}
	#endif
	if (ImGui::Button("Run Dialog Again", ImVec2(ImGui::GetContentRegionAvail().x,0))) show(DIALOG);

	#if defined(ANDROID) && defined(DEBUG)
	if (ImGui::Button("Reset Preferences", ImVec2(ImGui::GetContentRegionAvail().x,0)))
	{
		Preferences::reset_to_factory();
		doc.buttons.reshape(doc.camera);
		doc.redraw();
	}

	bool b0 = Preferences::cached_license(), b = b0;
	ImGui::Checkbox("Cached License (Debug!)", &b);
	if (b != b0)
	{
		Preferences::cached_license(b);
		reload_license();
	}
	b0 = ugly; b = b0;
	ImGui::Checkbox("Ugly License (Debug!)", &b);
	if (b != b0)
	{
		ugly = b;
		if (b) ugly_start_time = now();
	}
	#endif

	ImGui::Dummy(ImVec2(min.x, 0.25f*min.y));
	if (ImGui::Button("Close", ImVec2(ImGui::GetContentRegionAvail().x,0))) close();
}
