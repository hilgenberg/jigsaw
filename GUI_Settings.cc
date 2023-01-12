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

#define CHKBOOL(g, title, value, action) do{\
	bool on_ = (g), orig = on_ && value, tmp = orig;\
	enable(on_);\
	ImGui::Checkbox(title, &tmp);\
	if (enable && tmp != orig) w.action(); }while(0)

#define SLIDER_WITH_ID(g, id, title, value, min, max, action) do{\
	bool on_ = (g); double v0 = min, v1 = max;\
	auto orig = on_ ? value : min, tmp = orig;\
	enable(on_);\
	ImGui::SliderScalar("##" id, ImGuiDataType_Double, &tmp, &v0, &v1, title, slider_flags);\
	if (enable && tmp != orig) w.action(tmp); }while(0)

#define SLIDER(g, title, value, min, max, action) SLIDER_WITH_ID(g, title, title, value, min, max, action)

#define POPUP(g, title, T, value, action, ...) do{\
	static const char *items[] = {__VA_ARGS__};\
	bool on_ = (g); enable(on_);\
	int orig = on_ ? value : 0, tmp = orig;\
	ImGui::Combo(title, &tmp, items, IM_ARRAYSIZE(items));\
	if (enable && tmp != orig) w.action((T)tmp); }while(0)

#define COLOR(g, title, value, action) do{\
	bool on_ = (g);\
	GL_Color orig = on_ ? value : GL_Color(0.4), tmp = orig;\
	enable(on_);\
	ImGui::ColorEdit4(title, tmp.v, colorEditFlags);\
	if (enable && tmp != orig) w.action(tmp); }while(0)


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