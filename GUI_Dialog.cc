#include "GUI.h"
#include "Utility/Preferences.h"
#include "Utility/Timer.h"
#ifdef ANDROID
extern bool ugly; extern double ugly_start_time;
#else
// TODO: remove this after debugging on linux?
bool ugly = false;
double ugly_start_time = 0.0;
#endif

void GUI::p_dialog()
{
	ImVec2 min = ImGui::CalcTextSize("Testing Testing Testing Testing Testing");
	ImGui::PushTextWrapPos(min.x);

	static std::set<int> seen; seen.insert(dlg);
	if (trail.empty() || trail.back() != dlg) trail.push_back(dlg);
	const auto dlg0 = dlg;
	switch (dlg)
	{
		#define MSG(...)  ImGui::Text(__VA_ARGS__)
		#define ITEM(...) ImGui::Bullet(); ImGui::TextWrapped(__VA_ARGS__)
		#define BUY       do{ close(); buy_license(); }while(0)
		#define GO        dlg=
		#define NEXT      ++dlg
		#define ADD(n)    dlg += (n)
		#define EXIT      close()
		#define B(n,N,s)  \
			if (n > 1) ImGui::SameLine(); else ImGui::Dummy(ImVec2(min.x, 0.25f*min.y));\
			if (ImGui::Button(s, ImVec2(ImGui::GetContentRegionAvail().x / (float)(N-n+1), 0)))
		case 0:
			MSG("Welcome to the advanced features! To use them, please buy the full version of this app. That will give you all the following features:");
			ITEM("Up to 100.000 pieces");
			ITEM("The edge arranger, which finds all the edge and corner pieces for you and then neatly arranges everything");
			ITEM("The magnet tool, which moves several pieces at once by magnetizing the dragged pieces");
			ITEM("The shovel tool, which moves all pieces inside an area");
			ITEM("The hide tool, which moves a piece and all similarly colored pieces out of sight (use the arranger tools to get them back)");
			MSG("Also, supporting the development and maintenance of this app is highly appreciated!");
			B(1,1, "Sounds good. Take me to the store!") BUY;
			B(1,2, "Maybe later") EXIT;
			B(2,2, "I don't know") GO 2000;
			B(1,1, "Can't I get that for free?") GO 1000;
			if (seen.size() > 10) { B(1,1, "But I haven't seen all the dialogue!") NEXT; }
			break;
		case 1:
			#define TOTAL 30 // search for  ^\s*c[a]s(e) .+:  -- also check for N+1 breaks!
			assert(seen.size() <= TOTAL);
			if (seen.size() >= TOTAL)
			{
				MSG("In the 4 corners, in case you forgot. Nothing else to see here.");
				B(1,1, "Cool, I'll buy it now!") BUY;
				B(1,1, "Ok.") GO 0;
			}
			else if (seen.size() == TOTAL-1)
			{
				MSG("You've seen %d of %d already. The only page you're missing is the one with the hint about the secret menu in the full version. Want to see it?", (int)seen.size(), TOTAL);
				B(1,2, "Yes!") NEXT;
				B(2,2, "No!") GO 0;
			}
			else if (seen.size() < TOTAL-10)
			{
				MSG("You've seen %d out of %d already.", (int)seen.size(), TOTAL);
				B(1,1, "Ok") GO 0;
			}
			else
			{
				MSG("You've seen %d / %d - getting there.", (int)seen.size(), TOTAL);
				B(1,1, "Ok") GO 0;
			}
			break;
		case 2:
			MSG("It's in the 4 corners. But only in the full version, not the temporarily unlocked one. Nothing too fancy, just some debugging goodies, you might like to play with - I'll try to add something nice in every new version.");
			B(1,1, "Ok, guess I can buy it now!") BUY;
			B(1,1, "I'll use the ugly demo for a while.") GO 1303;
			B(1,1, "I'll use the smart demo for a while.") GO 1204;
			B(1,1, "Ok") EXIT;
			break;

		//-----------------------------------------------------------------------------------
		// 1000..1999: "Can't I get that for free?"
		//-----------------------------------------------------------------------------------
		case 1000:
			MSG("Why on earth would you get that for free?");
			B(1,1,"Because I'm pretty") GO 1100;
			B(1,1,"Because I'm smart")  GO 1200;
			B(1,1,"Because I'm ugly")   GO 1300;
			B(1,1,"Because I'm stupid") GO 1400;
			break;

		case 1100:
			MSG("Good for you! Good for everybody that can see you. But this app can't see you.");
			B(1,1, "Ok, I'll buy it.") BUY;
			B(1,1, "Maybe later.") EXIT;
			break;

		//----------------------------------------------------------------------------------

		case 1200:
			MSG("Are you sure you don't want to not buy it?");
			B(1,2, "Yes") BUY;
			B(2,2, "No") NEXT;
			break;
		case 1201:
			MSG("You're not unsure you don't want to not buy anything else?");
			B(1,2, "Yes") NEXT;
			B(2,2, "No") BUY;
			break;
		case 1202:
			MSG("But when you're not saying that you're not so sure, but still somewhat sure that you don't want to not buy it anytime but now, you're lying, right?");
			B(1,2, "Yes") NEXT;
			B(2,2, "No") BUY;
			break;
		case 1203:
			MSG("But what if nobody was to not say that you never wanted not to not definitely not buy it, would that be untrue?");
			B(1,2, "Yes") NEXT;
			B(2,2, "No") BUY;
			break;
		case 1204:
			MSG("Yes?");
			B(1,2, "No.") BUY;
			B(2,2, "Yes!") NEXT;
			break;
		case 1205:
			MSG("Well, alright, but you're not getting the proper full version with the secret menu, I'm just unlocking everything else for a while. Ok?");
			B(1,2, "Yes, do it!") { ugly = true; ugly_start_time = now(); NEXT; }
			B(2,2, "Nevermind.") GO 0;
			break;
		case 1206:
			MSG("Done.");
			B(1,2, "OK") EXIT;
			B(2,2, "Wait, undo!") { ugly = false; NEXT; }
			break;
		case 1207:
			MSG("Ok, undone.");
			B(1,1, "Ok") EXIT;
			break;

		//----------------------------------------------------------------------------------

		case 1300:
			MSG("Are you serious?");
			B(1,2, "No") GO 0;
			B(2,2, "Yes") NEXT;
			break;
		case 1301:
			MSG("Poor creature. Well, at least you're not stupid.");
			B(1,1, "Yes, that's something.") GO 0;
			B(1,1, "I'm stupid too.") NEXT;
			break;
		case 1302:
			MSG("Do you at least have friends?");
			B(1,1, "Yes, I do") GO 0;
			B(1,1, "No, nobody likes me") NEXT;
			break;
		case 1303: // jumping here from the completionist
			MSG("...");
			B(1,1, "...") NEXT;
			break;
		case 1304:
			MSG("Well, maybe solving some puzzles would be good for your self esteem. I can't give you the proper license but I could unlock all the features for a while.");
			B(1,2, "Yes, do it!") { ugly = true; ugly_start_time = now(); NEXT; }
			B(2,2, "Nevermind.") GO 0;
			break;
		case 1305:
			MSG("Ok, done.");
			B(1,2, "Thanks") EXIT;
			B(2,2, "Wait, undo!") { ugly = false; NEXT; }
			break;
		case 1306:
			MSG("Ok, undone.");
			B(1,1, "Ok") EXIT;
			break;

		//----------------------------------------------------------------------------------

		case 1400:
			MSG("Great! Well, all the experts say you should buy it!");
			B(1,1, "Oh, ok, I'll buy it!") BUY;
			B(1,1, "I don't believe that.") NEXT;
			break;
		case 1401:
			MSG("No, really, the science says so!");
			B(1,1, "Oh, ok, I'll buy it!") BUY;
			B(1,1, "I don't believe that.") NEXT;
			break;
		case 1402:
			MSG("But everybody else bought it!");
			B(1,1, "Oh, ok, I'll buy it too!") BUY;
			B(1,1, "I don't believe that.") NEXT;
			break;
		case 1403:
			MSG("Well, how stupid could you be then!");
			B(1,1, "...") EXIT;
			break;

		//-----------------------------------------------------------------------------------
		// 2000..2999: "I don't know"
		//-----------------------------------------------------------------------------------
		case 2000:
			MSG("If people actually buy this, I've got some neat features planned...");
			B(1,1,"Like what?") NEXT;
			B(1,1,"Will I get them for free?") GO 2100;
			B(1,1,"I don't care") EXIT;
			break;
		case 2001:
			MSG("Like rotation of the pieces and a new victory animation to go along with that.");
			MSG("Also, cropping of the images, so you can cut away the parts with no details.");
			B(1,1,"Anything else?") NEXT;
			B(1,1,"OK, I'll buy it") BUY;
			break;
		case 2002:
			MSG("Support for animated GIFs.");
			MSG("Maybe some sort of image search to find good high-resolution ones to use with higher piece counts.");
			B(1,1,"What else?") NEXT;
			B(1,1,"Cool, I'll buy it") BUY;
			break;
		case 2003:
			MSG("A new tool that finds all pieces that would fit into some empty spot, using gestures instead of the buttons, improve the hide tool, optionally draw borders and/or shadows around the pieces, ...");
			B(1,1,"What else?") NEXT;
			B(1,1,"Neat, I'll buy it") BUY;
			break;
		case 2004:
			MSG("If you have any ideas, that are still missing, use the contact button!");
			B(1,1,"OK") GO 0;
			break;

		case 2100: // "Will I get them for free?"
			MSG("Yes, this is like the shareware of old: just a one-time purchase - no subscriptions, no coins, no Skinner box manipulation (and by the way, also no spyware, notification spam or begging for likes and reviews), good for all further versions.");
			B(1,1,"OK, I'll buy it") BUY;
			B(1,1,"Maybe later") EXIT;
			B(1,1,"What else?") GO 2000;
			break;

		default:
			assert(false);
			MSG("How did you even get here?");
			B(1,1,"I don't know") EXIT;
			B(1,1,"I'll send a bug report!") EXIT;
			break;
	}
	ImGui::PopTextWrapPos();
	if (dlg != dlg0) doc.redraw(3);
}