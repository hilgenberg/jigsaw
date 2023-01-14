#include "imgui/imgui.h"
#include "GUI.h"
#include "Document.h"
#include "Utility/Preferences.h"
#include "data.h"

#ifdef LINUX
#include "imgui/backends/imgui_impl_sdl.h"
extern volatile bool quit;
#endif

void GUI::toggle() { visible = !visible; doc.redraw(3); }
void GUI::close() { visible = false; doc.redraw(3); }

void GUI::init_page()
{
	switch (page)
	{
		case PREFERENCES: break;
		case SETTINGS:
			tmp_N = doc.puzzle.N; break;
		case DIALOG:
			dlg = 0;
			break;
		default: assert(false);
	}
}

void GUI::draw()
{
	if (!visible) return;

	bool dark = Preferences::dark_mode();
	if (dark) ImGui::StyleColorsDark(); else ImGui::StyleColorsLight();
	ImGui::GetStyle().FrameBorderSize = dark ? 0.0f : 1.0f;

	ImGuiViewport &screen = *ImGui::GetMainViewport();
	ImGui::SetNextWindowBgAlpha(page==DIALOG ? 0.8f : 0.75f);
	ImVec2 center = screen.GetCenter();
	ImGui::SetNextWindowPos(center, page==DIALOG ? ImGuiCond_Always : ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
	//ImGui::SetNextWindowSize(screen.WorkSize, ImGuiCond_Always);

	if (ImGui::Begin(format("##GUI Page %d", page).c_str(), NULL, 
		ImGuiWindowFlags_AlwaysAutoResize | 
		ImGuiWindowFlags_NoTitleBar |
		ImGuiWindowFlags_NoResize |
		ImGuiWindowFlags_NoSavedSettings | 
		ImGuiWindowFlags_NoCollapse))
	{
		switch (page)
		{
			case PREFERENCES: p_preferences(); break;
			case SETTINGS: p_settings(); break;
			case DIALOG: p_dialog(); break;
			default: assert(false);
		}

		ImGui::End();
	}

	#ifdef DEBUG
	if (show_demo_window) ImGui::ShowDemoWindow(&show_demo_window);
	#endif
}

#ifdef LINUX
bool GUI::handle_event(const SDL_Event &event)
{
	bool h = ImGui_ImplSDL2_ProcessEvent(&event);
	if (visible && h) doc.redraw(3);
	ImGuiIO &io = ImGui::GetIO();

	bool activate = false, handled = false;

	if (handled)
	{}
	else if (event.type == SDL_QUIT)
	{
		quit = true;
		return true;
	}
	else if (event.type == SDL_KEYUP)
	{
		// workaround stuck modifier keys in imgui's SDL backend
		auto key = event.key.keysym.sym;
		switch (key)
		{
			case SDLK_LCTRL:
			case SDLK_RCTRL:
			{
				ImGuiIO& io = ImGui::GetIO();
    				io.AddKeyEvent(ImGuiMod_Ctrl, false);
				break;
			}
			case SDLK_LALT:
			case SDLK_RALT:
			{
				ImGuiIO& io = ImGui::GetIO();
    				io.AddKeyEvent(ImGuiMod_Alt, false);
				break;
			}
			case SDLK_LSHIFT:
			case SDLK_RSHIFT:
			{
				ImGuiIO& io = ImGui::GetIO();
    				io.AddKeyEvent(ImGuiMod_Shift, false);
				break;
			}
		}
	}
	else if (event.type == SDL_KEYDOWN)
	{
		auto key = event.key.keysym.sym;
		auto m   = event.key.keysym.mod;
		constexpr int SHIFT = 1, CTRL = 2, ALT = 4;
		const int  shift = (m & (KMOD_LSHIFT|KMOD_RSHIFT)) ? SHIFT : 0;
		const int  ctrl  = (m & (KMOD_LCTRL|KMOD_RCTRL)) ? CTRL : 0;
		const int  alt   = (m & (KMOD_LALT|KMOD_RALT)) ? ALT : 0;
		const int  mods  = shift + ctrl + alt;

		switch (key)
		{
			case SDLK_TAB:
			case SDLK_RETURN:
				if (mods || visible) break;
				activate = handled = true;
				break;

			case SDLK_ESCAPE:
				if (mods || ImGui::IsPopupOpen("", ImGuiPopupFlags_AnyPopupId + ImGuiPopupFlags_AnyPopupLevel))
					break;
				visible = !visible;
				doc.redraw(3);
				return true;

			case SDLK_q:
				if (mods == CTRL)
				{
					SDL_Event e; e.type = SDL_QUIT;
					SDL_PushEvent(&e);
					return true;
				}
				break;

			#ifdef DEBUG
			case SDLK_d:
				show_demo_window = !show_demo_window;
				doc.redraw(3);
				return true;
			case SDLK_f:
				show(DIALOG);
				return true;
			#endif
		}
	}

	if (activate)
	{
		visible = true;
		doc.redraw(3);
	}

	if (visible) switch (event.type)
	{
		case SDL_KEYDOWN: case SDL_KEYUP:
			if (io.WantCaptureKeyboard) return true;
			break;
		case SDL_MOUSEMOTION: case SDL_MOUSEBUTTONDOWN:
		case SDL_MOUSEBUTTONUP: case SDL_MOUSEWHEEL:
			if (io.WantCaptureMouse) return true;
			break;
	}
	return handled;
}
#else
bool GUI::handle_touch(int ds, int n, int *id, float *x, float *y)
{
	if (!visible) return false;
	doc.redraw(3);
	ImGuiIO &io = ImGui::GetIO();
	if (n <= 0) { io.AddMouseButtonEvent(0, false); return true; }
	// TODO: at least keep track of how many fingers are down
	io.AddMousePosEvent(*x, *y);
	io.AddMouseButtonEvent(0, ds >= 0);
	return true;
}
#endif
