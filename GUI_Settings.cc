#include "GUI.h"

static constexpr int slider_flags = ImGuiSliderFlags_AlwaysClamp|ImGuiSliderFlags_NoRoundToFormat|ImGuiSliderFlags_NoInput;

void GUI::p_settings()
{
	static bool applied = true;

	double m = 20.0, M = Preferences::Nmax(), orig = tmp_N;
	if (tmp_N < m) tmp_N = m;
	if (tmp_N > M) tmp_N = M;
	ImGui::SliderScalar("##N", ImGuiDataType_Double, &tmp_N, &m, &M, "%.0f Pieces", slider_flags);
	if (tmp_N != orig) applied = false;

	bool apply_ = false, close_ = false;
	if (ImGui::Button("OK", ImVec2(ImGui::GetContentRegionAvail().x*0.33,0))) { apply_ = !applied; close_ = true; }
	ImGui::SameLine();
	if (ImGui::Button(applied ? "Reset" : "Apply", ImVec2(ImGui::GetContentRegionAvail().x*0.5,0))) apply_ = true;
	ImGui::SameLine();
	if (ImGui::Button("Close", ImVec2(ImGui::GetContentRegionAvail().x,0))) close_ = true;

	if (apply_)
	{
		doc.load((int)tmp_N);
		doc.redraw();
		applied = true;
	}
	if (close_) close();
}