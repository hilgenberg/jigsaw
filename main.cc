#ifdef LINUX

#include <signal.h>
#include "Window.h"
#include "Renderer.h"
#include "Utility/Preferences.h"
#include "Utility/GL_Util.h"
#include "GUI.h"
#include "Audio.h"
#include <SDL.h>
#include <SDL_opengl.h>

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

int main(int argc, char *argv[])
{
	Preferences::reset();

	if (!audio_init(argc, argv))
	{
		fprintf(stderr, "ALUT init failed! Audio will be broken.\n");
	}

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

	if (SDL_Init(SDL_INIT_VIDEO) != 0)
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
	SDL_Window* window = SDL_CreateWindow("Jigsaw Puzzle", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 1280, 720, window_flags);
	SDL_GLContext gl_context = SDL_GL_CreateContext(window);
	SDL_GL_MakeCurrent(window, gl_context);
	SDL_GL_SetSwapInterval(1);

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
		if (file_arg)
		{
			std::filesystem::path p(file_arg);
			p = absolute(p);
			if (!doc.load(p.c_str(), n))
				throw std::runtime_error("Can't read image");
			Preferences::image(p.c_str());
		}
		else if (n > 0)
		{
			std::string p = Preferences::image();
			if (p.empty()) p = "///sample-data";
			if (!doc.load(p, n))
				throw std::runtime_error("Can't read image");
		}
		else
		{
			if (!Preferences::load_state(doc))
				doc.load("///sample-data", n);
		}

		if (n > 0) Preferences::pieces(n);

		GL_CHECK;
		Window w(window, doc);
		Renderer renderer(doc, w);
		new GUI(window, gl_context, doc);

		GL_CHECK;

		while (!quit)
		{
			GL_CHECK;
			SDL_Event event;
			while (!quit && SDL_PollEvent(&event))
			{
				if (GUI::gui->handle_event(event)) continue;
				if (w.handle_event(event)) continue;
			}
			if (quit) break;

			GL_CHECK;

			int W, H; SDL_GL_GetDrawableSize(window, &W, &H);
			GL_CHECK;
			w.reshape(W, H);
			GL_CHECK;
	
			if (doc.needs_redraw())
			{
				renderer.draw();
				SDL_GL_SwapWindow(window);
			}
			else
			{
				constexpr double SLEEP_MIN = 1.0 / 1000, SLEEP_MAX = 1.0 / 15;
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

	delete GUI::gui;
	SDL_GL_DeleteContext(gl_context);
	SDL_DestroyWindow(window);
	SDL_Quit();
	audio_quit();
	
	if (retcode == 0)
	{
		Preferences::save_state(doc);
		Preferences::flush();
	}

	return retcode;
}

#endif

