#include "GUI.h"
#include "Utility/Preferences.h"

static constexpr int slider_flags = ImGuiSliderFlags_AlwaysClamp|ImGuiSliderFlags_NoRoundToFormat|ImGuiSliderFlags_NoInput;

void GUI::p_settings()
{
	static bool applied = true;

	double m = 20.0, M = Preferences::Nmax(), orig = tmp_N;
	if (tmp_N < m) tmp_N = m;
	if (tmp_N > M) tmp_N = M;

	bool over = (tmp_N >= 300.0 && !license()); if (over)
	{
		ImGui::PushStyleColor(ImGuiCol_FrameBg,        (ImVec4)ImColor::HSV(0.11f, 0.6f, 0.6f));
		ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, (ImVec4)ImColor::HSV(0.11f, 0.7f, 0.7f));
		ImGui::PushStyleColor(ImGuiCol_FrameBgActive,  (ImVec4)ImColor::HSV(0.11f, 0.8f, 0.8f));
	}
	ImGui::SliderScalar("##N", ImGuiDataType_Double, &tmp_N, &m, &M, "%.0f Pieces", slider_flags);
	if (over) ImGui::PopStyleColor(3);
	if (tmp_N != orig) applied = false;

	bool apply_ = false, close_ = false;
	if (ImGui::Button("OK", ImVec2(ImGui::GetContentRegionAvail().x*0.33,0))) { apply_ = !applied; close_ = true; }
	ImGui::SameLine();
	if (ImGui::Button(applied ? "Reset" : "Apply", ImVec2(ImGui::GetContentRegionAvail().x*0.5,0))) apply_ = true;
	ImGui::SameLine();
	if (ImGui::Button("Close", ImVec2(ImGui::GetContentRegionAvail().x,0))) close_ = true;

	if (apply_)
	{
		if (tmp_N > 301.0 && !license())
		{
			show(DIALOG);
			return;
		}
		doc.load((int)tmp_N);
		doc.redraw();
		applied = true;
	}
	if (close_) close();
}