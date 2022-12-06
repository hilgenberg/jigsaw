#include <signal.h>
#include "PlotWindow.h"
#include "Utility/Preferences.h"
#include "GUI.h"
#include <SDL.h>
#include <SDL_opengl.h>
#include <SDL_mixer.h>

volatile bool quit = false;
static void signalHandler(int)
{
	SDL_Event e; e.type = SDL_QUIT;
	SDL_PushEvent(&e);
}

static int usage(const char *arg0)
{
	for (const char *s = arg0; *s; ++s) if (*s == '/') arg0 = s+1;
	printf("Usage: %s [IMAGE FILE] [NUMBER OF PIECES]\n", arg0);
	return 1;
}

static bool load(Document &doc, const char *file, int n = -1)
{
	if (file)
	{
		std::filesystem::path p(file);
		p = absolute(p);

		try
		{
			doc.load(p.c_str(), n);
		}
		catch (...)
		{
			return false;
		}
		return true;
	}
	std::filesystem::path p = Preferences::directory();
	if (!is_directory(p)) return false;
	p /= "state";
	if (!is_regular_file(p)) return false;

	FILE *F = fopen(p.c_str(), "r");
	if (!F)
	{
		//fprintf(stderr, "cannot read from config file %s!\n", cf.c_str());
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
		return false;
	}
	fclose(F);
	return true;
}
static bool save(Document &doc)
{
	std::filesystem::path p = Preferences::directory();
	if (!is_directory(p)) return false;
	p /= "state";

	FILE *F = fopen(p.c_str(), "w");
	if (!F)
	{
		fprintf(stderr, "cannot write to savegame file %s!\n", p.c_str());
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

int main(int argc, char *argv[])
{
	Preferences::reset();

	const char *file_arg = NULL;
	int n = -1;

	for (int i = 1; i < argc; ++i)
	{
		const char *s = argv[i];
		if (n < 0 && is_int(s, n))
		{
			if (n <= 0)
			{
				printf("Number of pieces must be positive\n");
				return 1;
			}
		}
		else if (!file_arg)
		{
			file_arg = s;
		}
		//if (!strcasecmp(s, "--help") || !strcmp(s, "-h")) return usage(argv[0]);
		else
		{
			return usage(argv[0]);
		}
	}

	
	struct sigaction sa;
	memset(&sa, 0, sizeof(struct sigaction));
	sa.sa_handler = signalHandler;
	sigaction(SIGINT,  &sa, 0);
	sigaction(SIGPIPE, &sa, 0);
	sigaction(SIGQUIT, &sa, 0);
	sigaction(SIGTERM, &sa, 0);

	if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_AUDIO) != 0)
	{
		fprintf(stderr, "Error: %s\n", SDL_GetError());
		return -1;
	}

	// Setup window
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 5);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
	SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_RED_SIZE,   8);
	SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE,  8);
	SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE, 8);
	SDL_WindowFlags window_flags = (SDL_WindowFlags)(SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);
	SDL_Window* window = SDL_CreateWindow("Puzzle", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 1280, 720, window_flags);
	SDL_GLContext gl_context = SDL_GL_CreateContext(window);
	SDL_GL_MakeCurrent(window, gl_context);
	SDL_GL_SetSwapInterval(Preferences::vsync());

	//Initialize SDL_mixer
	if (Mix_OpenAudio(MIX_DEFAULT_FREQUENCY, MIX_DEFAULT_FORMAT, MIX_DEFAULT_CHANNELS, 512) < 0)
	{
		fprintf(stderr, "SDL audio error: %s\n", Mix_GetError());
	}

	if (glewInit() != GLEW_OK)
	{
		fprintf(stderr, "GLEW init failed!\n");
		SDL_GL_DeleteContext(gl_context);
		SDL_DestroyWindow(window);
		SDL_Quit();
		return -1;
	}

	GL_CHECK;

	int retcode = 0;
	Document doc;

	try
	{
		if (!load(doc, file_arg, n))
		#ifdef DEBUG
		if (!load(doc, "test.jpg", n))
		#endif
		{
			throw std::runtime_error("File not found");
		}

		GL_CHECK;
		GL_Context context(gl_context);
		PlotWindow w(window, context, doc);
		GUI gui(window, gl_context, w);

		GL_CHECK;

		while (!quit)
		{
			GL_CHECK;
			SDL_Event event;
			while (!quit && SDL_PollEvent(&event))
			{
				if (gui.handle_event(event)) continue;
				if (w.handle_event(event)) continue;
			}
			if (quit) break;

			GL_CHECK;

			int W, H; SDL_GL_GetDrawableSize(window, &W, &H);
			GL_CHECK;
			w.reshape(W, H);
			GL_CHECK;
	
			if (w.animating() || w.needs_redraw() || gui.needs_redraw())
			{
				gui.update();
				GL_CHECK;
				w.animate();
				GL_CHECK;
				w.draw();
				GL_CHECK;
				gui.draw();
				GL_CHECK;
				SDL_GL_SwapWindow(window);
			}
			else
			{
				w.waiting();
				constexpr double SLEEP_MIN = 1.0 / 1000, SLEEP_MAX = 1.0 / 30;
				double st = SLEEP_MIN;
				while (!SDL_WaitEventTimeout(NULL, (int)(st*1000)))
				{
					if (quit) break; // signal handler can set it
					st *= 2.0;
					if (st > SLEEP_MAX) st = SLEEP_MAX;
				}
			}
		}
	}
	catch(std::exception &e)
	{
		fprintf(stderr, "%s\n", e.what());
		retcode = 2;
	}

	SDL_GL_DeleteContext(gl_context);
	SDL_DestroyWindow(window);
	Mix_CloseAudio();
	Mix_Quit();
	SDL_Quit();
	
	save(doc);
	Preferences::flush();

	return retcode;
}
