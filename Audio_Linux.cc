#ifdef LINUX
#include "Utility/Preferences.h"
#include <AL/al.h>
#include <AL/alc.h>
#include <AL/alut.h>
#include "data.h"

static ALuint buffer = 0, source = 0;

#define TEST_ERROR(_msg) do {\
	ALCenum error = alGetError();\
	if (error != AL_NO_ERROR) fprintf(stderr, "OpenAL Error: " _msg "\n");\
	}while(0)

bool audio_init(int &argc, char **argv)
{
	if (!alutInit(&argc, argv)) return false;

	ALfloat orientation[] = { 0.0f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f };
	alListener3f(AL_POSITION, 0, 0, 1.0f); TEST_ERROR("listener position");
    	alListener3f(AL_VELOCITY, 0, 0, 0); TEST_ERROR("listener velocity");
	alListenerfv(AL_ORIENTATION, orientation); TEST_ERROR("listener orientation");
	alGenSources((ALuint)1, &source); TEST_ERROR("source generation");
	alSourcef(source, AL_PITCH, 1); TEST_ERROR("source pitch");
	alSourcef(source, AL_GAIN, 0.2f); TEST_ERROR("source gain");
	alSource3f(source, AL_POSITION, 0, 0, 0); TEST_ERROR("source position");
	alSource3f(source, AL_VELOCITY, 0, 0, 0); TEST_ERROR("source velocity");
	alSourcei(source, AL_LOOPING, AL_FALSE); TEST_ERROR("source looping");
	buffer = alutCreateBufferFromFileImage(click_data, click_data_len);
	alSourcei(source, AL_BUFFER, buffer); TEST_ERROR("buffer binding");
	return true;
}

void play_click()
{
	if (!Preferences::click()) return;
	alSourcePlay(source); TEST_ERROR("source playing");
}

void audio_quit()
{
	alDeleteSources(1, &source); source = 0;
	alDeleteBuffers(1, &buffer); buffer = 0;
}
#endif