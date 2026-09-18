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

static void menu_adj(Engine *e, int tab, int item, int dir)
{
    float d = (float)dir;
    if (tab == 0) {
        if (item == 0) {
            e->throttle += d * 0.05f;
            if (e->throttle < 0)
                e->throttle = 0;
            if (e->throttle > 1)
                e->throttle = 1;
        } else if (item == 1) {
            e->mix += d * 0.05f;
            if (e->mix < 0.5f)
                e->mix = 0.5f;
            if (e->mix > 1.5f)
                e->mix = 1.5f;
        } else if (item == 2)
            e->ign ^= 1;
    } else if (tab == 1) {
        e->spark_deg += d * 1.f;
        if (e->spark_deg < 0)
            e->spark_deg = 0;
        if (e->spark_deg > 40)
            e->spark_deg = 40;
    } else if (tab == 2) {
        if (item == 0) {
            e->atmo += d * 2000.f;
            if (e->atmo < 70000.f)
                e->atmo = 70000.f;
            if (e->atmo > 110000.f)
                e->atmo = 110000.f;
        } else if (item == 1) {
            e->tamb += d * 5.f;
            if (e->tamb < 253.f)
                e->tamb = 253.f;
            if (e->tamb > 323.f)
                e->tamb = 323.f;
        }
    } else if (tab == 3) {
        if (item == 0) {
            e->cr += d * 0.5f;
            eng_rebuild(e);
        } else if (item == 1) {
            e->bore += d * 0.002f;
            if (e->bore < 0.06f)
                e->bore = 0.06f;
            if (e->bore > 0.12f)
                e->bore = 0.12f;
            eng_rebuild(e);
        }
    }
}

static int menu_nitem(int tab)
{
    if (tab == 0)
        return 3;
    if (tab == 1)
        return 1;
    if (tab == 2)
        return 2;
    return 2;
}

int main(void)
{
    Engine e;
    int run = 1, menu = 0, tab = 0, item = 0;
    float zoom = 1.f;
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
                else if (c == '\t') {
                    menu ^= 1;
                    item = 0;
                } else if (c == 0x1b) {
                    unsigned char seq[4] = {0};
                    int m = input_read(seq, 4);
                    if (m >= 2 && seq[0] == '[') {
                        if (menu) {
                            if (seq[1] == 'C')
                                menu_adj(&e, tab, item, 1);
                            else if (seq[1] == 'D')
                                menu_adj(&e, tab, item, -1);
                            else if (seq[1] == 'B' && item + 1 < menu_nitem(tab))
                                item++;
                            else if (seq[1] == 'A' && item)
                                item--;
                        }
                    } else if (menu)
                        menu = 0;
                    i = n;
                } else if (menu && (c == 'a' || c == 'A')) {
                    tab = (tab + 3) % 4;
                    item = 0;
                } else if (menu && (c == 'd' || c == 'D')) {
                    tab = (tab + 1) % 4;
                    item = 0;
                } else if (c == 'b' || c == 'B' || c == '\n' || c == '\r') {
                    e.autostart = 1;
                    e.ign = 1;
                    e.start_t = 0;
                    e.starter = 1;
                    log_line("autostart");
                } else if (c == '+' || c == '=') {
                    zoom *= 1.15f;
                    if (zoom > 2.4f)
                        zoom = 2.4f;
                } else if (c == '-' || c == '_') {
                    zoom /= 1.15f;
                    if (zoom < 0.45f)
                        zoom = 0.45f;
                } else if (c == 'i' || c == 'I')
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
                } else if (c == ' ')
                    e.starter = 1;
            }
        }

        if (e.autostart) {
            e.ign = 1;
            if (e.rpm < 1100.f)
                e.starter = 1;
            else {
                e.starter = 0;
                e.autostart = 0;
                log_line("autostart done");
            }
        }

        for (k = 0; k < 8; k++)
            eng_step(&e, 1.f / 240.f);

        fill(0, 0, (int)FB_W, (int)FB_H, rgb565(18, 20, 28));
        engine_draw(&e, zoom);
        hud_draw(&e);
        if (menu)
            menu_draw(&e, tab, item);
        audio_chunk(&e, 1.f / 30.f);
        usleep(33000);
    }
    audio_close();
    input_close();
    fb_close();
    log_close();
    return 0;
}
