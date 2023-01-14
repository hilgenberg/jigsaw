#pragma once

#ifdef LINUX
bool audio_init(int &argc, char **argv);
void audio_quit();
#else
void audio_pause();
extern bool pending_vibration;
#endif

void play_click();
