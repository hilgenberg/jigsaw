#pragma once

#ifdef LINUX
bool audio_init(int &argc, char **argv);
void audio_quit();
#else
void audio_pause();
#endif

void play_click();
