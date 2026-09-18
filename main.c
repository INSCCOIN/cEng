#include "app.h"
#include "engine.h"
#include "fb.h"
#include <stdio.h>
#include <unistd.h>

void input_open(void);
void input_close(void);
int input_read(unsigned char *buf, int n);
void log_init(void);
void log_line(const char *s);
void log_close(void);

int main(void)
{
    Engine e;
    int run = 1;
    if (fb_open() < 0) {
        fprintf(stderr, "cEng needs /dev/fb0\n");
        return 1;
    }
    log_init();
    eng_init(&e);
    input_open();
    audio_open();
    log_line("ready");
    while (run) {
        unsigned char b[16];
        int n = input_read(b, 16);
        int i, k;
        e.starter = 0;
        if (n > 0) {
            for (i = 0; i < n; i++) {
                unsigned char c = b[i];
                if (c == 'q' || c == 'Q')
                    run = 0;
                else if (c == 'i' || c == 'I')
                    e.ign ^= 1;
                else if (c == 'r' || c == 'R') {
                    eng_reset(&e);
                    log_line("reset");
                } else if (c == 'w' || c == 'W') {
                    e.throttle += 0.05f;
                    if (e.throttle > 1)
                        e.throttle = 1;
                } else if (c == 's' || c == 'S') {
                    e.throttle -= 0.05f;
                    if (e.throttle < 0)
                        e.throttle = 0;
                }
                if (c == ' ')
                    e.starter = 1;
            }
        }

        /* 8 physics substeps per frame ~30 Hz * 8 = 240 Hz */
        for (k = 0; k < 8; k++)
            eng_step(&e, 1.f / 240.f);

        fill(0, 0, (int)FB_W, (int)FB_H, rgb565(18, 20, 28));
        engine_draw(&e);
        hud_draw(&e);
        audio_chunk(&e, 1.f / 30.f);
        usleep(33000);
    }
    audio_close();
    input_close();
    fb_close();
    log_close();
    return 0;
}
