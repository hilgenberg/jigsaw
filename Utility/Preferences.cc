#include "Preferences.h"
#include "StringFormatting.h"
#include <pwd.h>
#include <thread>
using namespace std::filesystem;
static bool load();
static bool save();
static bool have_changes = false; // were any defaults changed?

#ifdef LINUX
static bool        vsync_        = true;
static int         fps_          = 60;
static std::string image_;
static int         pieces_       = 256;
#endif

static float       alpha_        = 0.0f;
static bool        absmode_      = false;
static EdgeType    edge_         = Regular;
static float       button_scale_ = 0.0f;
static bool        spiral_       = false;
static ScreenEdge  button_edge_  = LEFT;
static ScreenAlign button_align_ = TOP_OR_LEFT;
static GL_Color    bg_(0.25);

void reset_all()
{
	#ifdef LINUX
	vsync_        = true;
	fps_          = 60;
	pieces_       = 256;
	image_.clear();
	#endif
	edge_         = Regular;
	alpha_        = 0.0f;
	absmode_      = false;
	button_scale_ = 0.0f;
	spiral_       = false;
	button_edge_  = LEFT;
	button_align_ = TOP_OR_LEFT;
	bg_ = GL_Color(0.25);
	have_changes = false;
}


namespace Preferences
{
	path directory()
	{
		static path cfg;
		if (cfg.empty())
		{
			const char *home = getenv("HOME");
			if (!home) {
				passwd *pw = getpwuid(geteuid());
				if (pw) home = pw->pw_dir;
			}
			if (!home) return cfg;

			path h = weakly_canonical(home);
			auto cfg1 = h / ".config/puzzle", cfg2 = h / ".puzzle";
			cfg = is_directory(cfg1) ? cfg1 : is_directory(cfg2) ? cfg2 : 
				is_directory(h / ".config") ? cfg1 : cfg2;
			create_directories(cfg);
		}
		return cfg;
	}

	bool reset()
	{
		reset_all();
		load();
		return true;
	}
	bool flush() { return !have_changes || save(); }

	#define SET(x) do{ if ((x)==value) return; (x) = value; have_changes = true; }while(0)

	#ifdef LINUX
	
	bool vsync() { return vsync_; }
	void vsync(bool value) { SET(vsync_); }

	int  fps() { return fps_; }
	void fps(int value) { SET(fps_); }
	
	int  pieces() { return pieces_; }
	void pieces(int value) { SET(pieces_); }

	std::string image() { return image_; }
	void image(const std::string &value) { SET(image_); }

	#endif

	GL_Color bg_color() { return bg_; }
	void bg_color(const GL_Color &value) { SET(bg_); }

	float solution_alpha() { return alpha_; }
	void solution_alpha(float value) { SET(alpha_); }

	bool absolute_mode() { return absmode_; }
	void absolute_mode(bool value) { SET(absmode_); }

	bool spiral() { return spiral_; }
	void spiral(bool value) { SET(spiral_); }

	float button_scale() { return button_scale_; }
	void button_scale(float value) { SET(button_scale_); }
	ScreenEdge button_edge() { return (ScreenEdge)button_edge_; }
	void  button_edge(ScreenEdge value) { SET(button_edge_); }
	ScreenAlign button_align() { return (ScreenAlign)button_align_; }
	void  button_align(ScreenAlign value) { SET(button_align_); }

	EdgeType edge() { return edge_; }
	void edge(EdgeType value) { SET(edge_); }
};


static path config_file()
{
	static path cfg = Preferences::directory();
	if (cfg.empty()) return cfg;
	return cfg / "prefs.bin";
}

static bool load()
{
	path cf = config_file(); if (cf.empty()) return false;
	if (!is_regular_file(cf)) return false;
	FILE *file = fopen(cf.c_str(), "r");
	if (!file)
	{
		fprintf(stderr, "cannot read from config file %s!\n", cf.c_str());
		return false;
	}
	try
	{
		FileReader fr(file);
		Deserializer s(fr);
		#ifdef LINUX
		s.int32_(fps_);
		s.bool_(vsync_);
		s.int32_(pieces_);
		s.string_(image_);
		#endif
		s.float_(alpha_);
		s.bool_(absmode_);
		s.bool_(spiral_);
		s.enum_(edge_, None, Circle);
		s.float_(button_scale_);
		s.enum_(button_edge_, LEFT, BOTTOM);
		s.enum_(button_align_, TOP_OR_LEFT, BOTTOM_OR_RIGHT);
	}
	catch (...)
	{
		reset_all();
		fclose(file);
		return false;
	}

	fclose(file);
	have_changes = false;
	return true;
}

static bool save()
{
	path cf = config_file(); if (cf.empty()) return false;
	FILE *file = fopen(cf.c_str(), "w");
	if (!file)
	{
		fprintf(stderr, "cannot write to config file %s!\n", cf.c_str());
		return false;
	}
	try
	{
		FileWriter fw(file);
		Serializer s(fw);
		#ifdef LINUX
		s.int32_(fps_);
		s.bool_(vsync_);
		s.int32_(pieces_);
		s.string_(image_);
		#endif
		s.float_(alpha_);
		s.bool_(absmode_);
		s.bool_(spiral_);
		s.enum_((int)edge_, None, Circle);
		s.float_(button_scale_);
		s.enum_((int)button_edge_, LEFT, BOTTOM);
		s.enum_((int)button_align_, TOP_OR_LEFT, BOTTOM_OR_RIGHT);
	}
	catch (...)
	{
		fclose(file);
		return false;
	}
	fclose(file);
	have_changes = false;
	return true;
}

