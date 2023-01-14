#include "GUI.h"
#include "License.h"

void GUI::p_dialog()
{
	ImVec2 min = ImGui::CalcTextSize("Testing Testing Testing Testing Testing");
	ImGui::PushTextWrapPos(min.x);

	const auto dlg0 = dlg;
	switch (dlg)
	{
		#define MSG(s) ImGui::Text(s)
		#define B(n,N,s) \
			if (n > 1) ImGui::SameLine(); else ImGui::Dummy(ImVec2(min.x, 0.25f*min.y));\
			if (ImGui::Button(s, ImVec2(ImGui::GetContentRegionAvail().x / (float)(N-n+1), 0)))
		#define BUY do{ buy_license(); close(); }while(0)
		#define GOTO dlg=
		#define NEXT ++dlg
		#define EXIT close()
		case 0:
			MSG("Welcome to the advanced features! To use them, please buy the full version of this app. "
			"That will give you all the following features:");
			ImGui::Bullet(); ImGui::TextWrapped("Up to 100.000 pieces");
			ImGui::Bullet(); ImGui::TextWrapped("The edge arranger, which finds all the edge and corner pieces for you and then neatly arranges everything");
			ImGui::Bullet(); ImGui::TextWrapped("The magnet tool, which moves several pieces at once by magnetizing the dragged pieces");
			ImGui::Bullet(); ImGui::TextWrapped("The shovel tool, which moves all pieces inside a circle");
			ImGui::Bullet(); ImGui::TextWrapped("The hide tool, which moves a piece and all similarly colored pieces out of sight");
			MSG("Also, supporting the development and maintenance of this app would be highly appreciated!");
			B(1,1,"Sounds good. Take me to the store!") BUY;
			B(1,2,"Maybe later") EXIT;
			B(2,2,"Maybe never") GOTO 20;
			B(1,1,"Can't I get that for free?") GOTO 10;
			B(1,2,"I don't know") close();
			B(2,2,"I do like it...") close();
			break;
		
		case 10:
			MSG("Absolutely not! Why would I want to give you a free license?");
			B(1,1,"Because I'm pretty") NEXT;
			B(1,1,"Because I'm smart")  NEXT;
			B(1,1,"Because I'm ugly")   NEXT;
			B(1,1,"Because I'm stupid") NEXT;
			break;
		case 11:
			MSG("TODO");
			break;

		case 20:
			MSG("That's just mean.");
			B(1,1,"You're right") GOTO 0;
			B(1,1,"No, it's not") NEXT;
			break;
		case 21:
			MSG("Yes, it is.");
			B(1,1,"You're right") GOTO 0;
			B(1,1,"No, it's not") NEXT;
			break;
		case 22:
			MSG("It is.");
			B(1,1,"No, it's not") --dlg;
			B(1,1,"You're right") GOTO 0;
			break;

		default:
			assert(false);
			MSG("How did you even get here?");
			B(1,1,"I don't know") EXIT;
			B(1,1,"I don't care") EXIT;
			break;
	}
	ImGui::PopTextWrapPos();
	if (dlg != dlg0) doc.redraw(3);
}