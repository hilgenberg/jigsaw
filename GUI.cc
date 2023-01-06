#ifdef LINUX
#include "GUI.h"
#include "imgui/imgui.h"
#include "imgui/backends/imgui_impl_sdl.h"
#include "imgui/backends/imgui_impl_opengl3.h"
#include "Window.h"
#include "Utility/Preferences.h"

extern volatile bool quit;
extern const std::vector<unsigned char> &font_data();
static std::string ini_location;

GUI::GUI(SDL_Window* window, SDL_GLContext context, Window &w)
: w(w)
, visible(false)
, need_redraw(1)
{
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO &io = ImGui::GetIO();
	//io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls
	io.ConfigInputTextCursorBlink = false;
	io.ConfigInputTrickleEventQueue = false;

	auto p = Preferences::directory();
	if (p.empty()) io.IniFilename = NULL; else {
		::ini_location = p / "imgui.ini";
		io.IniFilename = ::ini_location.c_str();
	}
	ImGui_ImplSDL2_InitForOpenGL(window, context);
	ImGui_ImplOpenGL3_Init();

	auto &fd = font_data();
	ImFontConfig fc; fc.FontDataOwnedByAtlas = false;
	static const ImWchar ranges[] = { 0x0001, 0xFFFF, 0 }; // get all
	io.Fonts->AddFontFromMemoryTTF((void *)fd.data(), (int)fd.size(), 18.0f, &fc, ranges);
}

GUI::~GUI()
{
	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplSDL2_Shutdown();
	ImGui::DestroyContext();
}

void GUI::update()
{
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
	else if (!visible) return;

	bool dark = false; // TODO: go by image color
	if (dark) ImGui::StyleColorsDark(); else ImGui::StyleColorsLight();

	ImGui_ImplOpenGL3_NewFrame();
	ImGui_ImplSDL2_NewFrame();
	ImGui::NewFrame();

	ImGui::GetStyle().FrameBorderSize = dark ? 0.0f : 1.0f;

	// Always center preference window when appearing
	ImVec2 center = ImGui::GetMainViewport()->GetCenter();
	ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
	ImGui::Begin("Preferences", NULL, ImGuiWindowFlags_AlwaysAutoResize|ImGuiWindowFlags_NoCollapse);
	ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x);
	bool b0, b; int i0, i;
	
	static const char *edges[] = {"Square Tiles", "Regular Jigsaw", "Triangular Edge", "Rectangular Edge", "Semicircle"};
	i = i0 = Preferences::edge();
	ImGui::Combo("##Edges", &i, edges, 5);
	if (i != i0) Preferences::edge((EdgeType)i);

	float f0 = Preferences::solution_alpha(), f = f0;
	ImGui::SliderFloat("##Solution Alpha", &f, 0.0f, 1.0f, "Solution Alpha: %.3f", ImGuiSliderFlags_Logarithmic | ImGuiSliderFlags_AlwaysClamp | ImGuiSliderFlags_NoRoundToFormat | ImGuiSliderFlags_NoInput);
	if (f != f0) Preferences::solution_alpha(f);

	b0 = Preferences::absolute_mode(); b = b0;
	ImGui::Checkbox("Absolute Mode", &b);
	if (b != b0) Preferences::absolute_mode(b);

	b0 = Preferences::spiral(); b = b0;
	ImGui::Checkbox("Spiral Arrange", &b);
	if (b != b0) Preferences::spiral(b);

	ImGui::Spacing();
	ImGui::Spacing();
	ImGui::Spacing();
	ImGui::Spacing();
	ImGui::Spacing();
	
	f0 = Preferences::button_scale(); f = f0;
	ImGui::SliderFloat("##Button Scale", &f, -1.0f, 1.0f, "Button Scale: %.3f", ImGuiSliderFlags_AlwaysClamp | ImGuiSliderFlags_NoRoundToFormat | ImGuiSliderFlags_NoInput);
	if (f != f0)
	{
		Preferences::button_scale(f);
		w.doc.buttons.reshape(w.doc.camera);
	}
	
	static const char *button_edges[] = {"Left", "Right", "Top", "Bottom"};
	i = i0 = Preferences::button_edge();
	ImGui::Combo("##Button Placement", &i, button_edges, 4);
	if (i != i0) { Preferences::button_edge((ScreenEdge)i); w.doc.buttons.reshape(w.doc.camera); }
	bool button_v = (i == LEFT || i == RIGHT);
	
	static const char *button_align_h[] = {"Left", "Center", "Right"};
	static const char *button_align_v[] = {"Top", "Center", "Bottom"};
	i = i0 = Preferences::button_align();
	ImGui::Combo("##Button Alignment", &i, button_v ? button_align_v : button_align_h, 3);
	if (i != i0) { Preferences::button_align((ScreenAlign)i); w.doc.buttons.reshape(w.doc.camera); }

	ImGui::Spacing();
	ImGui::Spacing();
	ImGui::Spacing();
	ImGui::Spacing();
	ImGui::Spacing();

	ImGui::Text("Animation FPS limit (-1 for none)");
	i0 = Preferences::fps(); i = i0;
	ImGui::InputInt("##fps", &i, 1, 0);
	if (i != i0) Preferences::fps(i);

	b0 = Preferences::vsync(); b = b0;
	ImGui::Checkbox("VSync", &b);
	if (b != b0) {
		SDL_GL_SetSwapInterval(b);
		Preferences::vsync(b); redraw();
	}
	ImGui::PopItemWidth();
	ImGui::End();

	#ifdef DEBUG
	if (show_demo_window) ImGui::ShowDemoWindow(&show_demo_window);
	#endif
}

void GUI::draw()
{
	if (!visible) return;
	ImGui::Render();
	glUseProgram(0);
	ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
	if (need_redraw > 0) --need_redraw;
}

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
#endif