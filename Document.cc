#include "Document.h"
#include "Persistence/Serializer.h"
#include "Utility/Preferences.h"
#include "Puzzle_Tools.h"

//------------------------------------------------------------------
// construction and drawing
//------------------------------------------------------------------


Document::Document()
{
	buttons.reshape(camera);
}

bool Document::load(const std::string &p, int N)
{
	#ifdef LINUX
	if (N <= 0) N = Preferences::pieces();
	#endif
	if (!puzzle.load(p)) return false;
	puzzle.reset(puzzle.im.w(), puzzle.im.h(), N);
	puzzle.shuffle(false);
	arrange(puzzle, false, false);
	reset_view(puzzle, camera);
	return true;
}
bool Document::load(int N)
{
	puzzle.reset(puzzle.im.w(), puzzle.im.h(), N);
	puzzle.shuffle(false);
	arrange(puzzle, false, false);
	reset_view(puzzle, camera);
	return true;
}

void Document::load(Deserializer &s)
{
	s.member_(puzzle);
	s.member_(camera);
	s.marker_("EOF.");
	tool = Tool::NONE;
	if (puzzle.solved())
	{
		puzzle.reset(puzzle.im.w(), puzzle.im.h(), puzzle.N);
		puzzle.shuffle(false);
	}
}
void Document::save(Serializer &s) const
{
	s.member_(puzzle);
	s.member_(camera);
	s.marker_("EOF.");
}
