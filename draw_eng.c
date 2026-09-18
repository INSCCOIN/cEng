#include "engine.h"
#include "fb.h"
#include <math.h>
#include <stdio.h>

void engine_draw(const Engine *e)
{
    const float sc = 1400.f; /* px per metre */
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
    text(6, (int)FB_H - 14, "W/S thr  I ign  Space starter  R reset  Q", rgb565(160, 160, 170));
    if (e->starter)
        text((int)FB_W - 80, (int)FB_H - 14, "CRANK", rgb565(255, 200, 40));
}
