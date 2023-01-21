#include "GUI.h"
#include "Utility/Preferences.h"
#include "data.h"

#define SPC for (int i = 0; i < 5; ++i) ImGui::Spacing()

static constexpr int colorEditFlags = 
	ImGuiColorEditFlags_NoOptions | ImGuiColorEditFlags_NoInputs |
	ImGuiColorEditFlags_NoTooltip | //ImGuiColorEditFlags_AlphaBar |
	ImGuiColorEditFlags_PickerHueWheel;

static void Help(const char *desc)
{
	#ifdef LINUX
	if (ImGui::IsItemActive() || !ImGui::IsItemHovered()) return;
	#else
	if (!ImGui::IsItemHovered()) return;
	#endif
	ImGui::BeginTooltip();
	ImGui::PushTextWrapPos(ImGui::GetFontSize() * 20.0f);
	ImGui::TextUnformatted(desc);
	ImGui::PopTextWrapPos();
	ImGui::EndTooltip();
}

void GUI::p_preferences()
{
	ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x);
	bool b0, b; int i0, i; float f0, f;
	
	static const char *edges[] = {"Square Tiles", "Regular Jigsaw", "Triangular Edge", "Rectangular Edge", "Semicircle"};
	i = i0 = Preferences::edge();
	ImGui::Combo("##Edges", &i, edges, 5);
	if (i != i0) Preferences::edge((EdgeType)i);

	#ifdef DEBUG
	constexpr int NUM_OPTIONS = 7;
	#else
	constexpr int NUM_OPTIONS = 6;
	#endif
	static const char *s_max_pieces[] = {"Max Pieces: 500", "Max Pieces: 1000", "Max Pieces: 5000", "Max Pieces: 10.000", "Max Pieces: 50.000", "Max Pieces: 100.000", "Max Pieces: 1.000.000"};
	static const int   n_max_pieces[] = { 500 ,  1000 ,  5000 ,   10000,    50000,    100000,     1000000 };
	int NN = Preferences::Nmax();
	for (i = 0; i+1 < NUM_OPTIONS; ++i) if (n_max_pieces[i] >= NN) break;
	i0 = i;
	ImGui::Combo("##N_MAX", &i, s_max_pieces, NUM_OPTIONS);
	if (i != i0) Preferences::Nmax(n_max_pieces[i]);

	f0 = Preferences::solution_alpha(); f = f0;
	ImGui::SliderFloat("##Solution Alpha", &f, 0.0f, 1.0f, "Solution Visibility", ImGuiSliderFlags_Logarithmic|ImGuiSliderFlags_AlwaysClamp | ImGuiSliderFlags_NoRoundToFormat | ImGuiSliderFlags_NoInput);
	if (f != f0) Preferences::solution_alpha(f);

	b0 = Preferences::absolute_mode(); b = b0;
	ImGui::Checkbox("Snap to Solution", &b);
	if (b != b0) Preferences::absolute_mode(b);

	GL_Color orig = Preferences::bg_color(), tmp = orig;
	ImGui::ColorEdit4("Background Color", tmp.v, colorEditFlags);
	if (tmp != orig) { Preferences::bg_color(tmp); doc.redraw(); }

	b0 = Preferences::click(); b = b0;
	ImGui::Checkbox("Click When Connecting", &b);
	if (b != b0) Preferences::click(b);

	#ifdef ANDROID
	b0 = Preferences::vibrate(); b = b0;
	ImGui::Checkbox("Vibrate When Connecting", &b);
	if (b != b0) Preferences::vibrate(b);
	#endif

	b0 = Preferences::spiral(); b = b0;
	ImGui::Checkbox("Spiral Arrange", &b);
	if (b != b0) Preferences::spiral(b);

	b0 = Preferences::hide_help(); b = b0;
	ImGui::Checkbox("Hide the Help Button", &b);
	if (b != b0) { Preferences::hide_help(b); doc.buttons.reshape(doc.camera); }

	SPC;
	
	ImVec2 min = ImGui::CalcTextSize("Button Scale: +123.456 XXX");
	ImGui::SetNextItemWidth(min.x);
	f0 = Preferences::button_scale(); f = f0;
	ImGui::SliderFloat("##Button Scale", &f, -1.0f, 1.0f, "Button Scale: %.2f", ImGuiSliderFlags_AlwaysClamp | ImGuiSliderFlags_NoRoundToFormat | ImGuiSliderFlags_NoInput);
	if (f != f0)
	{
		Preferences::button_scale(f);
		doc.buttons.reshape(doc.camera);
	}
	
	//ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x*0.5f);
	static const char *button_edges[] = {"Left", "Right", "Top", "Bottom"};
	i = i0 = Preferences::button_edge();
	ImGui::Combo("##Button Placement", &i, button_edges, 4);
	if (i != i0) { Preferences::button_edge((ScreenEdge)i); doc.buttons.reshape(doc.camera); }
	bool button_v = (i == LEFT || i == RIGHT);
	
	//ImGui::SameLine();
	//ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
	static const char *button_align_h[] = {"Left", "Center", "Right"};
	static const char *button_align_v[] = {"Top", "Center", "Bottom"};
	i = i0 = Preferences::button_align();
	ImGui::Combo("##Button Alignment", &i, button_v ? button_align_v : button_align_h, 3);
	if (i != i0) { Preferences::button_align((ScreenAlign)i); doc.buttons.reshape(doc.camera); }

	SPC;

	f0 = Preferences::finger_radius(); f = f0;
	ImGuiIO &io = ImGui::GetIO();
	float mf = 0.5f*0.25*std::min(io.DisplaySize.x, io.DisplaySize.y);
	ImGui::SliderFloat("##Finger Radius", &f, 0.0f, mf, "Finger Radius", ImGuiSliderFlags_AlwaysClamp | ImGuiSliderFlags_NoRoundToFormat | ImGuiSliderFlags_NoInput);
	Help("All pieces intersecting this circle are considered during hit testing.");
	if (f != f0) Preferences::finger_radius(f);
	if (ImGui::IsItemActive())
	{
		ImVec2 p = ImGui::GetCursorScreenPos();
		p.x += 4.0f + f;
		p.y += 4.0f + f;
		ImGui::GetWindowDrawList()->AddCircle(p, f, ImColor(1.0f, 1.0f, 0.4f), 0, 3.0f);
		ImGui::Dummy(ImVec2(f * 2.0f + 8.0f, f * 2.0f + 10.0f));
	}

	#ifdef ANDROID
	b0 = Preferences::adaptive_touch(); b = b0;
	ImGui::Checkbox("Adaptive Hit Test", &b);
	if (b != b0) Preferences::adaptive_touch(b);
	Help("Keeps track of where you tend to grab the pieces (like mostly on their bottom-right, f.e.) and favors the pieces near that position. If you want to train it, pick up some lone piece about 5 times the way you want it to think is optimal.");
	#endif

	SPC;

	if (ImGui::Button("Done", ImVec2(ImGui::GetContentRegionAvail().x, 0)))
	{
		Preferences::flush();
		close();
	}

	ImGui::PopItemWidth();
}