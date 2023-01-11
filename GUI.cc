#ifdef LINUX
#include "imgui/imgui.h"
#include "imgui/backends/imgui_impl_sdl.h"
#include "imgui/backends/imgui_impl_opengl3.h"
#else
#include <android/native_window_jni.h>
#include "imgui/imgui.h"
#include "imgui/backends/imgui_impl_android.h"
#define IMGUI_IMPL_OPENGL_ES3
#include "imgui/backends/imgui_impl_opengl3.h"
#endif

#include "GUI.h"
#include "Window.h"
#include "Utility/Preferences.h"
#include "data.h"

extern volatile bool quit;

GUI *GUI::gui = NULL;

#ifdef LINUX
GUI::GUI(SDL_Window* window, SDL_GLContext context, Window &w)
: w(w)
#else
GUI::GUI(ANativeWindow *window, Window &w)
: w(w)
, window(window)
#endif
{
	assert(!gui); gui = this;

	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO &io = ImGui::GetIO();
	io.ConfigInputTextCursorBlink = false;
	io.ConfigInputTrickleEventQueue = false;
	io.IniFilename = NULL;
	#ifdef LINUX
	ImGui_ImplSDL2_InitForOpenGL(window, context);
	ImGui_ImplOpenGL3_Init();
	float font_size = 18.0f;
	#else
	ImGui_ImplAndroid_Init(window);
	ImGui_ImplOpenGL3_Init("#version 300 es");
	ImGui::GetStyle().ScaleAllSizes(3.0f); // FIXME: Put some effort into DPI awareness
	float font_size = 42.0f;
	#endif

	ImFontConfig fc; fc.FontDataOwnedByAtlas = false;
	static const ImWchar ranges[] = { 0x0001, 0xFFFF, 0 }; // get all
	io.Fonts->AddFontFromMemoryTTF((void *)font_data, (int)font_data_len, font_size, &fc, ranges);
}
	
GUI::~GUI()
{
	assert(gui == this); gui = NULL;

	ImGui_ImplOpenGL3_Shutdown();
	#ifdef LINUX
	ImGui_ImplSDL2_Shutdown();
	#else
	ImGui_ImplAndroid_Shutdown();
	#endif
	ImGui::DestroyContext();
	#ifdef ANDROID
	ANativeWindow_release(window);
	#endif
}

void GUI::toggle() { visible = !visible; w.redraw(); }
void GUI::close() { visible = false; w.redraw(); }

void GUI::init_page()
{
	switch (page)
	{
		case PREFERENCES: break;
		case SETTINGS:
			tmp_N = w.doc.puzzle.N; break;
		default: assert(false);
	}
}

void GUI::draw()
{
	#ifdef LINUX
	if (ImGui::GetIO().MouseDrawCursor != visible)
	{
		ImGui::GetIO().MouseDrawCursor = visible;

		if (!visible)
		{
			// if gui is visible, regular drawing will apply the cursor state,
			// but if not, draw one empty frame to update the state
			ImGui_ImplOpenGL3_NewFrame();
			ImGui_ImplSDL2_NewFrame();
			ImGui::NewFrame();
			ImGui::Render();
			ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
			ImGui::EndFrame();

			return;
		}
	}
	else
	#endif
	if (!visible) return;

	bool dark = Preferences::dark_mode();
	if (dark) ImGui::StyleColorsDark(); else ImGui::StyleColorsLight();

	ImGui_ImplOpenGL3_NewFrame();
	#ifdef LINUX
	ImGui_ImplSDL2_NewFrame();
	#else
	ImGui_ImplAndroid_NewFrame();
	#endif
	ImGui::NewFrame();

	ImGui::GetStyle().FrameBorderSize = dark ? 0.0f : 1.0f;

	ImGuiViewport &screen = *ImGui::GetMainViewport();
	ImGui::SetNextWindowBgAlpha(0.75f);
	ImVec2 center = screen.GetCenter();
	ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
	//ImGui::SetNextWindowSize(screen.WorkSize, ImGuiCond_Always);

	ImGui::Begin(format("##GUI Page %d", page).c_str(), NULL, 
		ImGuiWindowFlags_AlwaysAutoResize | 
		ImGuiWindowFlags_NoTitleBar |
		ImGuiWindowFlags_NoResize |
		ImGuiWindowFlags_NoSavedSettings | 
		ImGuiWindowFlags_NoCollapse);

	switch (page)
	{
		case PREFERENCES: p_preferences(); break;
		case SETTINGS: p_settings(); break;
		default: assert(false);
	}

	ImGui::End();

	#ifdef DEBUG
	if (show_demo_window) ImGui::ShowDemoWindow(&show_demo_window);
	#endif

	ImGui::Render();
	glUseProgram(0);
	glDisable(GL_DEPTH_TEST);
	#ifdef ANDROID
	ImGuiIO &io = ImGui::GetIO();
	glViewport(0, 0, (int)io.DisplaySize.x, (int)io.DisplaySize.y);
	#endif
	ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
	if (need_redraw > 0) --need_redraw;
}

#ifdef LINUX
bool GUI::handle_event(const SDL_Event &event)
{
	bool h = ImGui_ImplSDL2_ProcessEvent(&event);
	if (visible && h) redraw();
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
				redraw();
				w.redraw();
				return true;

			case SDLK_q:
				if (mods == CTRL)
				{
					SDL_Event e; e.type = SDL_QUIT;
					SDL_PushEvent(&e);
					return true;
				}
				break;
		}
	}

	if (activate)
	{
		visible = true;
		redraw();
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
	redraw();
	ImGuiIO &io = ImGui::GetIO();
	if (n <= 0) { io.AddMouseButtonEvent(0, false); return true; }
	// TODO: at least keep track of how many fingers are down
	io.AddMousePosEvent(*x, *y);
	io.AddMouseButtonEvent(0, ds >= 0);
	return true;
}
#endif
