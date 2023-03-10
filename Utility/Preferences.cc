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
static std::string image_;
static int         pieces_         = 256;
#define DEFAULT_FINGER 0.0f
#else
#include "Timer.h"
#define DEFAULT_FINGER 60.0f
#endif

static float       solution_alpha_ = 0.0f;
static bool        absolute_mode_  = false;
static EdgeType    edge_           = Regular;
static int         Nmax_           = 1000;
static float       button_scale_   = 0.0f;
static bool        spiral_         = false;
static ScreenEdge  button_edge_    = LEFT;
static ScreenAlign button_align_   = CENTERED;
static GL_Color    bg_color_(0.25);
static float       finger_radius_  = DEFAULT_FINGER;
static bool        click_          = true;
static bool        hide_help_      = false;
#ifdef ANDROID
static bool        vibrate_        = false;
static bool        adaptive_touch_ = true;
#endif

static path cfg;

namespace Preferences
{
	void reset_to_factory()
	{
		#ifdef LINUX
		pieces_         = 256;
		image_          . clear();
		#endif
		edge_           = Regular;
		Nmax_           = 1000;
		solution_alpha_ = 0.0f;
		absolute_mode_  = false;
		button_scale_   = 0.0f;
		spiral_         = false;
		button_edge_    = LEFT;
		button_align_   = CENTERED;
		bg_color_       = GL_Color(0.25);
		finger_radius_  = DEFAULT_FINGER;
		click_          = true;
		hide_help_      = false;
		#ifdef ANDROID
		vibrate_        = false;
		adaptive_touch_ = true;
		#endif
	}

	bool save_state(const Document &doc)
	{
		std::filesystem::path p = directory();
		if (!is_directory(p)) return false;
		p /= "state.bin";

		FILE *F = fopen(p.c_str(), "w");
		if (!F) { LOG_ERROR("cannot write to savegame file %s", p.c_str()); return false; }

		bool ok = true; try
		{
			FileWriter fw(F);
			Serializer s(fw);
			doc.save(s);
		}
		catch (...) { LOG_ERROR("Error writing savegame %s!", p.c_str()); ok = false; }
		fclose(F);
		return ok;
	}

	bool load_state(Document &doc)
	{
		std::filesystem::path p = directory(); p /= "state.bin";
		if (!is_regular_file(p)) return false;
		
		FILE *F = fopen(p.c_str(), "r");
		if (!F) { LOG_ERROR("cannot read from savegame file %s", p.c_str()); return false; }
		
		bool ok = true; try
		{
			FileReader fr(F);
			Deserializer s(fr);
			doc.load(s);
			assert(s.done());
		}
		catch (...) {
			LOG_ERROR("Error reading savegame %s!", p.c_str());
			ok = false;
			doc.load("///sample-data", 150);
		}
		fclose(F);
		return ok;
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
		reset_to_factory();
		load();
		return true;
	}
	bool flush() { return !have_changes || save(); }

	#define SET(x) do{ if ((x)==value) return; (x) = value; have_changes = true; }while(0)
	#define PREFV(type, name) type name() { return name##_; } void name(type        value) { SET(name##_); }
	#define PREFR(type, name) type name() { return name##_; } void name(const type &value) { SET(name##_); }

	#ifdef LINUX
	PREFR(std::string, image);
	PREFV(int,         pieces);
	#endif

	PREFV(EdgeType,    edge);
	PREFV(int,         Nmax);
	PREFR(GL_Color,    bg_color);
	PREFV(float,       solution_alpha);
	PREFV(bool,        absolute_mode);
	PREFV(float,       button_scale);
	PREFV(ScreenEdge,  button_edge);
	PREFV(ScreenAlign, button_align);
	PREFV(bool,        spiral);
	PREFV(float,       finger_radius);
	PREFV(bool,        click);
	PREFV(bool,        hide_help);
	#ifdef ANDROID
	PREFV(bool,        vibrate);
	PREFV(bool,        adaptive_touch);
	#endif
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
		LOG_ERROR("Cannot read from preference file %s!\n", cf.c_str());
		return false;
	}
	try
	{
		FileReader fr(file);
		Deserializer s(fr);
		#ifdef LINUX
		s.int32_ (pieces_);
		s.string_(image_);
		#endif
		s.float_ (solution_alpha_);
		if (s.version() >= FILE_VERSION_1_1) bg_color_.load(s);
		s.bool_  (absolute_mode_);
		s.bool_  (spiral_);
		s.enum_  (edge_, None, Circle);
		s.int32_ (Nmax_);
		s.float_ (button_scale_);
		s.enum_  (button_edge_, LEFT, BOTTOM);
		s.enum_  (button_align_, TOP_OR_LEFT, BOTTOM_OR_RIGHT);
		s.float_ (finger_radius_);
		s.bool_  (click_);
		s.bool_  (hide_help_);
		#ifdef ANDROID
		s.bool_  (vibrate_);
		if (s.version() < FILE_VERSION_1_1) { int license; s.int32_(license); }
		s.bool_  (adaptive_touch_);
		#endif
	}
	catch (std::exception &e)
	{
		LOG_ERROR("Exception while reading from preference file %s: %s!\n", cf.c_str(), e.what());
		fclose(file);
		return false;
	}
	catch (...)
	{
		LOG_ERROR("Exception while reading from preference file %s!\n", cf.c_str());
		Preferences::reset_to_factory();
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
		LOG_ERROR("Cannot write to preference file %s!", cf.c_str());
		return false;
	}

	try
	{
		FileWriter fw(file);
		Serializer s(fw);
		#ifdef LINUX
		s.int32_ (pieces_);
		s.string_(image_);
		#endif
		s.float_ (solution_alpha_);
		if (s.version() >= FILE_VERSION_1_1) bg_color_.save(s);
		s.bool_  (absolute_mode_);
		s.bool_  (spiral_);
		s.enum_  ((int)edge_, None, Circle);
		s.int32_ (Nmax_);
		s.float_ (button_scale_);
		s.enum_  ((int)button_edge_, LEFT, BOTTOM);
		s.enum_  ((int)button_align_, TOP_OR_LEFT, BOTTOM_OR_RIGHT);
		s.float_ (finger_radius_);
		s.bool_  (click_);
		s.bool_  (hide_help_);
		#ifdef ANDROID
		s.bool_  (vibrate_);
		if (s.version() < FILE_VERSION_1_1) s.int32_(true /*cached_license_*/ ? 0x00040005 : 0x00010000);
		s.bool_  (adaptive_touch_);
		#endif
	}
	catch (std::exception &e)
	{
		LOG_ERROR("Exception while writing to preference file %s: %s!\n", cf.c_str(), e.what());
		fclose(file);
		return false;
	}
	catch (...)
	{
		LOG_ERROR("Exception while writing to preference file %s!\n", cf.c_str());
		fclose(file);
		return false;
	}
	fclose(file);
	have_changes = false;
	return true;
}

