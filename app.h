#ifndef CENG_APP_H
#define CENG_APP_H
#include "engine.h"
void audio_open(void);
void audio_close(void);
void audio_chunk(const Engine *e, float dt);
void engine_draw(const Engine *e, float zoom);
void hud_draw(const Engine *e);
void menu_draw(const Engine *e, int tab, int item);
#endif
