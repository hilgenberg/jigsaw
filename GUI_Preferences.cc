#include "GUI.h"
#include "imgui/imgui.h"
#include "Window.h"
#include "Utility/Preferences.h"
#include "data.h"

void GUI::p_preferences()
{
	ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x);
	bool b0, b; int i0, i;
	
	static const char *edges[] = {"Square Tiles", "Regular Jigsaw", "Triangular Edge", "Rectangular Edge", "Semicircle"};
	i = i0 = Preferences::edge();
	ImGui::Combo("##Edges", &i, edges, 5);
	if (i != i0) Preferences::edge((EdgeType)i);

	float f0 = Preferences::solution_alpha(), f = f0;
	//ImGuiSliderFlags_Logarithmic
	ImGui::SliderFloat("##Solution Alpha", &f, 0.0f, 1.0f, "Solution Visibility", ImGuiSliderFlags_AlwaysClamp | ImGuiSliderFlags_NoRoundToFormat | ImGuiSliderFlags_NoInput);
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
	
	//ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x*0.5f);
	static const char *button_edges[] = {"Left", "Right", "Top", "Bottom"};
	i = i0 = Preferences::button_edge();
	ImGui::Combo("##Button Placement", &i, button_edges, 4);
	if (i != i0) { Preferences::button_edge((ScreenEdge)i); w.doc.buttons.reshape(w.doc.camera); }
	bool button_v = (i == LEFT || i == RIGHT);
	
	//ImGui::SameLine();
	//ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
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

	if (ImGui::Button("Done", ImVec2(ImGui::GetContentRegionAvail().x, 0))) close();

	ImGui::PopItemWidth();
}