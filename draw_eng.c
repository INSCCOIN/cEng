#include "engine.h"
#include "fb.h"
#include <math.h>
#include <stdio.h>

void engine_draw(const Engine *e, float zoom)
{
    const float sc = 1400.f * zoom;
    int cx = (int)FB_W / 2;
    int cy = (int)FB_H / 2 + 36;
    float s = sinf(e->th), c = cosf(e->th);
    float pinx = e->r * s;
    float piny = -e->r * c; /* screen y down, TDC is up */
    float pis = e->r * c + sqrtf(e->rod * e->rod - e->r * e->r * s * s);
    int pxp = cx + (int)(pinx * sc);
    int pyp = cy + (int)(piny * sc);
    int piston_y = cy - (int)(pis * sc);
    int pw = (int)(e->bore * sc * 0.5f);
    uint16_t steel = rgb565(170, 175, 185);
    uint16_t pin = rgb565(220, 200, 80);
    uint16_t fire = rgb565(255, (int)(80 + e->fire_str * 160), 20);
    uint16_t wall = rgb565(90, 95, 110);
    uint16_t head = rgb565(70, 74, 88);
    int top = piston_y - 50;
    int bot = cy + (int)(e->r * sc) + 8;
    int i;

    /* block */
    rect(cx - pw - 6, top, (pw + 6) * 2, bot - top, wall);
    fill(cx - pw - 4, top + 1, 3, bot - top - 2, rgb565(50, 52, 60));
    fill(cx + pw + 1, top + 1, 3, bot - top - 2, rgb565(50, 52, 60));
    fill(cx - pw, top, pw * 2, 10, head);

    /* gas color in chamber */
    {
        float pr = e->p / 2.5e6f;
        if (pr > 1)
            pr = 1;
        if (pr < 0)
            pr = 0;
        fill(cx - pw + 2, top + 10, pw * 2 - 4, piston_y - 8 - (top + 10),
             e->fire_str > 0.15f ? fire : rgb565((int)(40 + pr * 180), (int)(40 + pr * 40), (int)(50 + (1 - pr) * 40)));
    }

    /* piston */
    fill(cx - pw + 2, piston_y - 8, pw * 2 - 4, 16, steel);
    rect(cx - pw + 2, piston_y - 8, pw * 2 - 4, 16, rgb565(40, 40, 40));

    /* rod */
    line(pxp, pyp, cx, piston_y, rgb565(200, 200, 210));
    line(pxp + 1, pyp, cx + 1, piston_y, rgb565(120, 120, 130));

    /* crank / flywheel */
    for (i = 0; i < 36; i++) {
        float a0 = i * (2.f * 3.14159f / 36.f);
        float a1 = (i + 1) * (2.f * 3.14159f / 36.f);
        line(cx + (int)(cosf(a0) * e->r * sc * 1.15f), cy + (int)(sinf(a0) * e->r * sc * 1.15f),
             cx + (int)(cosf(a1) * e->r * sc * 1.15f), cy + (int)(sinf(a1) * e->r * sc * 1.15f), steel);
    }
    line(cx, cy, pxp, pyp, pin);
    fill(pxp - 3, pyp - 3, 7, 7, pin);
    fill(cx - 3, cy - 3, 7, 7, rgb565(30, 30, 30));
}

void hud_draw(const Engine *e)
{
    static const char *stname[] = {"INTAKE", "COMP", "POWER", "EXHAUST"};
    char b[96];
    uint16_t wh = rgb565(230, 230, 230);
    uint16_t dim = rgb565(30, 32, 44);
    int st = eng_stroke(e);
    fill(0, 0, (int)FB_W, 36, dim);
    fill(0, (int)FB_H - 20, (int)FB_W, 20, dim);
    text(6, 4, "cEng", rgb565(255, 180, 60));
    snprintf(b, sizeof b, "%5.0f rpm", e->rpm);
    text(50, 4, b, wh);
    snprintf(b, sizeof b, "thr %.0f%%  %s  %s", e->throttle * 100.f, e->ign ? "IGN" : "CUT", stname[st]);
    text(6, 16, b, e->ign ? rgb565(120, 220, 120) : rgb565(220, 80, 80));
    snprintf(b, sizeof b, "P %.1f bar", e->p / 1e5f);
    text((int)FB_W - 90, 8, b, wh);
    {
        int tw = (int)(e->throttle * 120.f);
        fill((int)FB_W - 130, 22, 120, 8, rgb565(20, 20, 20));
        fill((int)FB_W - 130, 22, tw, 8, rgb565(80, 180, 80));
    }
    text(6, (int)FB_H - 14, "B start  Tab menu  +/- zoom  W/S thr  I ign  Q", rgb565(160, 160, 170));
    {
        uint16_t go = e->autostart ? rgb565(255, 200, 40) : rgb565(40, 160, 70);
        fill((int)FB_W - 78, (int)FB_H - 18, 72, 16, go);
        text((int)FB_W - 72, (int)FB_H - 14, e->autostart ? "CRANK" : "START", rgb565(10, 10, 10));
    }
}

void menu_draw(const Engine *e, int tab, int item)
{
    const char *tabs[] = {"Fuel", "Spark", "Air", "Geom"};
    char line[4][48];
    int i, x = 8, y = 40;
    uint16_t bg = rgb565(24, 28, 40), hi = rgb565(60, 90, 160), wh = rgb565(230, 230, 230);
    fill(4, 38, 220, 118, bg);
    rect(4, 38, 220, 118, rgb565(90, 110, 160));
    for (i = 0; i < 4; i++) {
        int w = 50;
        fill(x, y, w, 12, i == tab ? hi : rgb565(40, 44, 58));
        text(x + 4, y + 2, tabs[i], wh);
        x += 54;
    }
    if (tab == 0) {
        snprintf(line[0], 48, "throttle  %3.0f %%", e->throttle * 100.f);
        snprintf(line[1], 48, "mix       %.2f", e->mix);
        snprintf(line[2], 48, "ignition  %s", e->ign ? "on" : "cut");
        snprintf(line[3], 48, "");
    } else if (tab == 1) {
        snprintf(line[0], 48, "timing    %.0f deg BTDC", e->spark_deg);
        snprintf(line[1], 48, "");
        snprintf(line[2], 48, "");
        snprintf(line[3], 48, "");
    } else if (tab == 2) {
        snprintf(line[0], 48, "atmo      %.0f kPa", e->atmo / 1000.f);
        snprintf(line[1], 48, "air temp  %.0f C", e->tamb - 273.f);
        snprintf(line[2], 48, "weather   %s", e->atmo > 101000.f ? "high" : (e->atmo < 90000.f ? "thin" : "std"));
        snprintf(line[3], 48, "");
    } else {
        snprintf(line[0], 48, "comp      %.1f :1", e->cr);
        snprintf(line[1], 48, "bore      %.0f mm", e->bore * 1000.f);
        snprintf(line[2], 48, "");
        snprintf(line[3], 48, "");
    }
    for (i = 0; i < 4; i++) {
        if (!line[i][0])
            continue;
        if (i == item)
            fill(8, 56 + i * 14, 210, 13, hi);
        text(12, 58 + i * 14, line[i], wh);
    }
    text(12, 140, "arrows adj  Esc close", rgb565(140, 150, 170));
}
