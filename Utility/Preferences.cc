#include "Preferences.h"
#include "StringFormatting.h"
#include "../Document.h"
#include <pwd.h>
#include <thread>
using namespace std::filesystem;
static bool load();
static bool save();
static bool have_changes = false; // were any defaults changed?

#ifdef LINUX
static bool        vsync_          = true;
static int         fps_            = 60;
static std::string image_;
static int         pieces_         = 256;
#endif

static float       solution_alpha_ = 0.0f;
static bool        absolute_mode_  = false;
static EdgeType    edge_           = Regular;
static float       button_scale_   = 0.0f;
static bool        spiral_         = false;
static ScreenEdge  button_edge_    = LEFT;
static ScreenAlign button_align_   = TOP_OR_LEFT;
static GL_Color    bg_color_(0.25);

void reset_all()
{
	#ifdef LINUX
	vsync_          = true;
	fps_            = 60;
	pieces_         = 256;
	image_.clear();
	#endif
	edge_           = Regular;
	solution_alpha_ = 0.0f;
	absolute_mode_  = false;
	button_scale_   = 0.0f;
	spiral_         = false;
	button_edge_    = LEFT;
	button_align_   = TOP_OR_LEFT;
	bg_color_       = GL_Color(0.25);
	
	have_changes = false;
}

static path cfg;

namespace Preferences
{
	bool save_state(const Document &doc)
	{
		std::filesystem::path p = directory();
		if (!is_directory(p)) return false;
		p /= "state.bin";

		FILE *F = fopen(p.c_str(), "w");
		if (!F)
		{
			LOG_ERROR("cannot write to savegame file %s", p.c_str());
			return false;
		}

		try
		{
			FileWriter fw(F);
			Serializer s(fw);
			doc.save(s);
		}
		catch (...)
		{
			fclose(F);
			return false;
		}
		fclose(F);
		return true;
	}

	bool load_state(Document &doc)
	{
		std::filesystem::path p = directory(); p /= "state.bin";
		if (!is_regular_file(p)) return false;
		FILE *F = fopen(p.c_str(), "r");
		if (!F)
		{
			LOG_ERROR("cannot read from savegame file %s", p.c_str());
			return false;
		}
		try
		{
			FileReader fr(F);
			Deserializer s(fr);
			doc.load(s);
			assert(s.done());
		}
		catch (...)
		{
			fclose(F);
			LOG_ERROR("Error reading savegame %s!", p.c_str());
			return false;
		}
		fclose(F);
		return true;
	}

	#ifdef ANDROID
	void directory(const std::string &data_dir)
	{
		assert(cfg.empty() || data_dir == cfg.c_str());
		if (cfg.empty())
		{
			LOG_DEBUG("Prefs dir: %s", data_dir.c_str());
			cfg = data_dir;
			reset();
		}
	}
	#endif

	path directory()
	{
		#ifdef LINUX
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
		#endif
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
	#define PREFV(type, name) type name() { return name##_; } void name(type        value) { SET(name##_); }
	#define PREFR(type, name) type name() { return name##_; } void name(const type &value) { SET(name##_); }

	#ifdef LINUX
	PREFV(int,         fps);
	PREFV(bool,        vsync);
	PREFR(std::string, image);
	PREFV(int,         pieces);
	#endif

	PREFV(EdgeType,    edge);
	PREFR(GL_Color,    bg_color);
	PREFV(float,       solution_alpha);
	PREFV(bool,        absolute_mode);
	PREFV(float,       button_scale);
	PREFV(ScreenEdge,  button_edge);
	PREFV(ScreenAlign, button_align);
	PREFV(bool,        spiral);
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
		s.int32_ (fps_);
		s.bool_  (vsync_);
		s.int32_ (pieces_);
		s.string_(image_);
		#endif
		s.float_ (solution_alpha_);
		s.bool_  (absolute_mode_);
		s.bool_  (spiral_);
		s.enum_  (edge_, None, Circle);
		s.float_ (button_scale_);
		s.enum_  (button_edge_, LEFT, BOTTOM);
		s.enum_  (button_align_, TOP_OR_LEFT, BOTTOM_OR_RIGHT);
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
		s.int32_ (fps_);
		s.bool_  (vsync_);
		s.int32_ (pieces_);
		s.string_(image_);
		#endif
		s.float_ (solution_alpha_);
		s.bool_  (absolute_mode_);
		s.bool_  (spiral_);
		s.enum_  ((int)edge_, None, Circle);
		s.float_ (button_scale_);
		s.enum_  ((int)button_edge_, LEFT, BOTTOM);
		s.enum_  ((int)button_align_, TOP_OR_LEFT, BOTTOM_OR_RIGHT);
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

