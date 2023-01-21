#include "GUI.h"
#include "Utility/Preferences.h"
#include "imgui_internal.h"

static inline ImVec2 operator+(const ImVec2 &a, const ImVec2 &b) { return ImVec2(a.x+b.x, a.y+b.y); }
/// Draws vertical text. The position is the bottom left of the text rect.
static void AddTextVertical(ImDrawList *DrawList, const char *text, ImVec2 pos, ImU32 text_color) {
	pos.x = IM_ROUND(pos.x);
	pos.y = IM_ROUND(pos.y);
	ImFont *font = GImGui->Font;
	char c;
	while ((c = *text++)) {
		const ImFontGlyph *glyph = font->FindGlyph(c);
		if (!glyph) continue;
		DrawList->PrimReserve(6, 4);
		DrawList->PrimQuadUV(pos + ImVec2(glyph->Y0, -glyph->X0),
				     pos + ImVec2(glyph->Y0, -glyph->X1),
				     pos + ImVec2(glyph->Y1, -glyph->X1),
				     pos + ImVec2(glyph->Y1, -glyph->X0),

				     ImVec2(glyph->U0, glyph->V0),
				     ImVec2(glyph->U1, glyph->V0),
				     ImVec2(glyph->U1, glyph->V1),
				     ImVec2(glyph->U0, glyph->V1), text_color);
		pos.y -= glyph->AdvanceX;
	}
}

void GUI::p_help() {
	ImGuiViewport &screen = *ImGui::GetMainViewport();
	float W = screen.WorkSize.x, H = screen.WorkSize.y, w = 0.0f, h = 0.0f;
	const auto &B = doc.buttons.buttons;
	const int n = (int)B.size();
	auto *dl = ImGui::GetForegroundDrawList();
	bool dark = Preferences::dark_mode();
	auto color = dark ? IM_COL32(255, 255, 255, 255) : IM_COL32(0, 0, 0, 255);
	std::map<int, ImVec2> sizes;
	const auto edge = Preferences::button_edge();
	for (auto &b : B)
	{
		auto s = ImGui::CalcTextSize(b.help());
		sizes[b.index] = s;
		if (s.x > w) w = s.x;
		if (s.y > h) h = s.y;
	}
	float bh = doc.buttons.button_size.y*H*0.5f, bw = doc.buttons.button_size.x*W*0.5f;
	bool horz = (edge == LEFT || edge == RIGHT);
	bool vert = (edge == TOP || edge == BOTTOM);

	if (horz && h <= bh)
	{
		float dx  = (edge == LEFT ? bw*1.5f :  -bw*1.5f-w);
		float dxb = (edge == LEFT ? 1.0f : -1.0f)*(bw*0.5+h*0.5);
		for (auto &b : B)
		{
			P2d p = b.pos; p.x += 1.0; p.x *= 0.5*W; p.y -= 1.0; p.y *= -0.5*H;
			float x = p.x + dx;
			float y = p.y - h*0.5;
			dl->AddText(ImVec2(x,y), color, b.help());
			x += (edge == LEFT ? -h*0.5 : sizes[b.index].x+h*0.5);
			dl->AddLine(ImVec2(x, p.y), ImVec2(p.x + dxb, p.y), color, 1.5f);
		}
	}
	else if (vert && h <= bw)
	{
		float dy  = (edge == BOTTOM ?  -bh*1.5f :  bh*1.5f+w);
		float dyb = (edge == BOTTOM ? -1.0f : 1.0f)*(bh*0.5+h*0.5);
		for (auto &b : B)
		{
			P2d p = b.pos; p.x += 1.0; p.x *= 0.5*W; p.y -= 1.0; p.y *= -0.5*H;
			float x = p.x - h*0.5;
			float y = p.y + dy;
			AddTextVertical(dl, b.help(), ImVec2(x,y), color);
			y += (edge == BOTTOM ? h*0.5 : -sizes[b.index].x-h*0.5);
			dl->AddLine(ImVec2(p.x, y), ImVec2(p.x, p.y + dyb), color, 1.5f);
		}
	}
	else if (horz)
	{
		float x = (W-w)*0.5f, y = 0.0f;
		switch (Preferences::button_align())
		{
			case TOP_OR_LEFT: y = h*0.75; break;
			case CENTERED: y = (H - n*h - (n-1)*h*.75f)*0.5; break;
			case BOTTOM_OR_RIGHT: y = H-n*h*1.75f; break;
		}
		float dx  = (edge == LEFT ? -h*0.5 : w+h*0.5);
		float dxb = (edge == LEFT ? 1.0f : -1.0f)*(bw*0.5+h*0.5);
		for (auto &b : B)
		{
			P2d p = b.pos; p.x += 1.0; p.x *= 0.5*W; p.y -= 1.0; p.y *= -0.5*H;
			dl->AddText(ImVec2(x,y), color, b.help());
			dl->AddLine(ImVec2(x+dx, y + h*0.5f), ImVec2(p.x + dxb, p.y), color, 1.5f);
			if (edge == RIGHT) dl->AddLine(ImVec2(x+sizes[b.index].x+h*0.5, y + h*0.5f), ImVec2(x+dx, y + h*0.5f), color, 1.5f);
			y += h*1.75f;
		}
	}
	else
	{
		float x = 0.0f, y = (H+w)*0.5f;
		switch (Preferences::button_align())
		{
			case TOP_OR_LEFT: x = h*0.75; break;
			case CENTERED: x = (W - n*h - (n-1)*h*.75f)*0.5; break;
			case BOTTOM_OR_RIGHT: x = W-n*h*1.75f; break;
		}
		float dy  = (edge == BOTTOM ? h*0.5 : -w-h*0.5);
		float dyb = (edge == BOTTOM ? -1.0f : 1.0f)*(bh*0.5+h*0.5);
		for (auto &b : B)
		{
			P2d p = b.pos; p.x += 1.0; p.x *= 0.5*W; p.y -= 1.0; p.y *= -0.5*H;
			AddTextVertical(dl, b.help(), ImVec2(x,y), color);
			dl->AddLine(ImVec2(x+h*0.5f, y+dy), ImVec2(p.x, p.y+dyb), color, 1.5f);
			if (edge == TOP) dl->AddLine(ImVec2(x+h*0.5f, y-sizes[b.index].x-h*0.5), ImVec2(x+h*0.5f, y+dy), color, 1.5f);
			x += h*1.75f;
		}
	}
}
