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
		#define MSG(...) ImGui::Text(__VA_ARGS__)
		#define ITEM(...) ImGui::Bullet(); ImGui::TextWrapped(__VA_ARGS__)
		#define B(n,N,s) \
			if (n > 1) ImGui::SameLine(); else ImGui::Dummy(ImVec2(min.x, 0.25f*min.y));\
			if (ImGui::Button(s, ImVec2(ImGui::GetContentRegionAvail().x / (float)(N-n+1), 0)))
		#define BUY do{ buy_license(); close(); }while(0)
		#define GO dlg=
		#define NEXT ++dlg
		#define ADD(n) dlg += (n)
		#define EXIT close()
		case 0:
			MSG("Welcome to the advanced features! To use them, please buy the full version of this app. That will give you all the following features:");
			ITEM("Up to 100.000 pieces");
			ITEM("The edge arranger, which finds all the edge and corner pieces for you and then neatly arranges everything");
			ITEM("The magnet tool, which moves several pieces at once by magnetizing the dragged pieces");
			ITEM("The shovel tool, which moves all pieces inside a circle");
			ITEM("The hide tool, which moves a piece and all similarly colored pieces out of sight (then use the arranger tools to get them back)");
			MSG("Also, supporting the development and maintenance of this app is highly appreciated!");
			B(1,1, "Sounds good. Take me to the store!") BUY;
			B(1,2, "Maybe later") EXIT;
			B(2,2, "I don't know") GO 2000;
			B(1,1, "Can't I get that for free?") GO 1000;
			if (seen.size() > 10) { B(1,1, "But I haven't seen all the dialogue!") NEXT; }
			break;
		case 1:
			#define TOTAL 20 // search for  ^\s*c[a]s(e) .+:  -- also check for N+1 breaks!
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
			B(1,1, "Ok. But, I'll keep on pretending to be ugly.") GO 1303;
			B(1,1, "Hm. Ok.") EXIT;
			break;

		//---------------------------------------------------------------------------------------
		// 1000..1999: "Can't I get that for free?"
		//---------------------------------------------------------------------------------------
		case 1000:
			MSG("Absolutely not! Why would I give you a free license?");
			B(1,1,"Because I'm pretty") GO 1100;
			B(1,1,"Because I'm smart")  GO 1200;
			B(1,1,"Because I'm ugly")   GO 1300;
			B(1,1,"Because I'm stupid") GO 1400;
			break;

		case 1100:
			MSG("That really does not help you, when all you can do is click on buttons.");
			B(1,1, "Ok, I'll buy it.") BUY;
			B(1,1, "Yes, I'll buy it.") BUY;
			B(1,1, "Oh no! I'll buy it.") BUY;
			break;

		case 1200:
			MSG("Why don't you just crack it, then?");
			B(1,1, "I'm too smart to waste so much time.") BUY;
			B(1,1, "Maybe I'll do that, then.") NEXT;
			break;
		case 1201:
			MSG("Fine.");
			B(1,1, "Fine.") NEXT;
			break;
		case 1202:
			MSG("So, how are you going to do that?");
			B(1,1, "I'll attack the Kotlin/Java parts.") NEXT;
			B(1,1, "I'll attack the C++ parts.") NEXT;
			B(1,1, "I'll attack the Google servers.") NEXT;
			B(1,1, "I'll attack something else.") NEXT;
			B(1,1, "I've changed my mind.") BUY;
			break;
		case 1203:
			MSG("Well, good luck! Buy it anyway, tell me how it went ;-)");
			B(1,1, "OK") EXIT;
			break;

		case 1300:
			MSG("Are you serious?");
			B(1,2, "No") GO 0;
			B(2,2, "Yes") NEXT;
			break;
		case 1301:
			MSG("Poor creature. Well, at least you're not stupid...");
			B(1,1, "Yes, that's something...") GO 0;
			B(1,1, "I'm stupid too.") NEXT;
			break;
		case 1302:
			MSG("Do you at least have friends?");
			B(1,1, "Yes, I do :)") GO 0;
			B(1,1, "No, nobody likes me :(") NEXT;
			break;
		case 1303: // jumping here from the completionist
			MSG("...");
			B(1,1, "...") NEXT;
			break;
		case 1304:
			MSG("Well, maybe solving some puzzles would be good for your self esteem... I can't give you the proper license but I could unlock all the features for a while.");
			B(1,2, "Yes, do it!") { ugly = true; ugly_start_time = now(); NEXT; }
			B(2,2, "Nevermind.") GO 0;
			break;
		case 1305:
			MSG("Ok, done.");
			B(1,2, "Cool") EXIT;
			B(2,2, "Thanks") EXIT;
			B(1,1, "Haha, you idiot!") { ugly = false; NEXT; }
			break;
		case 1306:
			MSG("Pfff, undone.");
			B(1,2, ":(") EXIT;
			B(2,2, "Hmpf") EXIT;
			break;

		case 1400:
			MSG("Great! Well, the experts say you should buy it! They say you should buy it right now, actually!");
			B(1,1, "Oh, ok, I'll buy it!") BUY;
			B(1,1, "I don't care.") NEXT;
			B(1,1, "You're lying.") NEXT;
			break;
		case 1401:
			MSG("No, really, it's the science. Come on!");
			B(1,1, "Oh, ok, I'll buy it!") BUY;
			B(1,1, "Rubbish.") NEXT;
			break;
		case 1402:
			MSG("Well, how stupid can you be then!");
			B(1,3, "Hm") GO 0;
			B(2,3, "Err") GO 0;
			B(3,3, "...") GO 0;
			break;

		//---------------------------------------------------------------------------------------
		// 2000..2999: "I don't know"
		//---------------------------------------------------------------------------------------
		case 2000:
			MSG("If enough people buy it, I've got some neat features planned...");
			B(1,1,"Will I get them for free?") GO 2100;
			B(1,1,"Like what?") ADD(2);
			B(1,1,"I don't know") NEXT;
			break;
		case 2001:
			MSG("I don't know either...");
			B(1,1,"Oh no!") EXIT;
			B(1,1,"I don't know") ADD(-1);
			break;
		case 2002:
			MSG("Like rotation of the pieces and a new victory animation to go along with that.");
			B(1,1,"Anything else?") NEXT;
			B(1,1,"OK, I'll buy it") BUY;
			break;
		case 2003:
			MSG("Cropping of the images, so you can cut away the parts with no details.");
			B(1,1,"What else?") NEXT;
			B(1,1,"OK, I'll buy it") BUY;
			break;
		case 2004:
			MSG("Support for animated GIFs.");
			B(1,1,"What else?") NEXT;
			B(1,1,"Cool, I'll buy it") BUY;
			break;
		case 2005:
			MSG("Builtin image search to find good high-resolution ones to use.");
			B(1,1,"What else?") NEXT;
			B(1,1,"Neat, I'll buy it") BUY;
			break;
		case 2006:
			MSG("A new tool that finds all pieces that would fit into some empty spot, using gestures instead of the buttons, improve the hide tool, optionally draw borders and/or shadows around the pieces, ...");
			B(1,1,"What else?") NEXT;
			B(1,1,"Neat, I'll buy it") BUY;
			break;
		case 2007:
			MSG("If there's something else, that you're missing, send email to th@zoon.cc");
			B(1,1,"OK") EXIT;
			break;

		case 2100: // "Will I get them for free?"
			MSG("Yes, I don't like profit maximization at all costs. This is just a one-time purchase like in the shareware of old - no subscriptions, no coins, no Skinner-Box manipulation.");
			B(1,1,"OK, I'll buy it") BUY;
			B(1,1,"Maybe later") EXIT;
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