#include "engine.h"
#include "fb.h"
#include <math.h>
#include <stdio.h>

void engine_draw(const Engine *e, float zoom)
{
    const float sc = 520.f * zoom;
    int cx = (int)FB_W / 2;
    int cy = (int)FB_H / 2 + 28;
    int i;
    uint16_t steel = rgb565(186, 190, 198);
    uint16_t dark = rgb565(42, 46, 56);
    uint16_t gold = rgb565(220, 190, 70);
    uint16_t wall = rgb565(78, 84, 96);

    fill(cx - (int)(0.22f * sc), cy - (int)((e->rod + e->r) * sc) - 20,
         (int)(0.44f * sc), (int)((e->rod + 2 * e->r) * sc) + 36, rgb565(28, 30, 38));

    for (i = 0; i < NC; i++) {
        double pinx, piny, pis, cth;
        int pxp, pyp, py, bx;
        int pw;
        float pr;
        uint16_t gas;
        cth = e->th + e->cyl[i].phase;
        eng_piston(e, cth, &pinx, &piny, &pis);
        bx = cx + (int)((i - 1.5) * e->bore * 1.35 * sc);
        pxp = bx + (int)(pinx * sc);
        pyp = cy + (int)(piny * sc);
        py = cy - (int)(pis * sc);
        pw = (int)(e->bore * 0.5 * sc);
        if (pw < 4)
            pw = 4;
        rect(bx - pw - 3, py - 36, (pw + 3) * 2, cy + (int)(e->r * sc) + 8 - (py - 36), wall);
        pr = (float)(gas_P(&e->cyl[i].ch) / 3.5e6);
        if (pr > 1)
            pr = 1;
        if (pr < 0)
            pr = 0;
        gas = e->cyl[i].lit ? rgb565(255, 90, 20) : rgb565((int)(50 + pr * 180), (int)(45 + pr * 50), 55);
        if (py - 8 > py - 34)
            fill(bx - pw + 1, py - 34, pw * 2 - 2, (py - 8) - (py - 34), gas);
        fill(bx - pw + 1, py - 7, pw * 2 - 2, 14, steel);
        rect(bx - pw + 1, py - 7, pw * 2 - 2, 14, dark);
        line(pxp, pyp, bx, py, gold);
        line(pxp + 1, pyp, bx + 1, py, rgb565(90, 80, 40));
        fill(pxp - 2, pyp - 2, 5, 5, gold);
    }
    {
        int k;
        int rw = (int)(e->r * sc);
        for (k = 0; k < 32; k++) {
            float a0 = k * (2.f * 3.14159f / 32.f);
            float a1 = (k + 1) * (2.f * 3.14159f / 32.f);
            line(cx + (int)(cosf(a0) * rw * 1.2f), cy + (int)(sinf(a0) * rw * 1.2f),
                 cx + (int)(cosf(a1) * rw * 1.2f), cy + (int)(sinf(a1) * rw * 1.2f), steel);
        }
        fill(cx - 3, cy - 3, 7, 7, dark);
    }
}

void hud_draw(const Engine *e)
{
    char b[96];
    uint16_t face = rgb565(16, 18, 28), wh = rgb565(235, 235, 240);
    uint16_t acc = rgb565(255, 170, 40), good = rgb565(70, 200, 90);
    int tw;
    fill(0, 0, (int)FB_W, 40, face);
    fill(0, (int)FB_H - 22, (int)FB_W, 22, face);
    text(6, 4, "cEng  I4", acc);
    snprintf(b, sizeof b, "%5.0f", e->rpm);
    text((int)FB_W / 2 - 24, 6, b, wh);
    text((int)FB_W / 2 + 20, 6, "rpm", rgb565(140, 140, 150));
    snprintf(b, sizeof b, "P %.1f bar   T %.0f C   AFR %.2f", e->p / 1e5f, e->t - 273.f,
             e->fuel > 0.001f ? (0.21f / e->fuel) / 12.5f : 0);
    text(6, 18, b, rgb565(180, 185, 195));
    tw = (int)(e->throttle * 100);
    fill((int)FB_W - 120, 8, 100, 8, rgb565(30, 30, 30));
    fill((int)FB_W - 120, 8, tw, 8, good);
    text((int)FB_W - 120, 20, e->ign ? "IGN" : "CUT", e->ign ? good : rgb565(220, 70, 70));
    {
        uint16_t go = (e->starter || e->autostart) ? acc : good;
        fill((int)FB_W - 86, (int)FB_H - 20, 80, 16, go);
        text((int)FB_W - 78, (int)FB_H - 16, e->starter ? "CRANK" : "B START", rgb565(10, 10, 12));
    }
    text(6, (int)FB_H - 16, "B start  Tab  +/-  W/S  I  Q", rgb565(150, 155, 165));
}

void menu_draw(const Engine *e, int tab, int item)
{
    const char *tabs[] = {"Fuel", "Spark", "Air", "Geom"};
    char line[4][48];
    int i, x = 8, y = 44;
    uint16_t bg = rgb565(20, 24, 36), hi = rgb565(50, 80, 150), wh = rgb565(230, 230, 230);
    fill(4, 42, 228, 122, bg);
    rect(4, 42, 228, 122, rgb565(80, 100, 150));
    for (i = 0; i < 4; i++) {
        fill(x, y, 52, 12, i == tab ? hi : rgb565(36, 40, 54));
        text(x + 4, y + 2, tabs[i], wh);
        x += 56;
    }
    line[0][0] = line[1][0] = line[2][0] = line[3][0] = 0;
    if (tab == 0) {
        snprintf(line[0], 48, "throttle  %3.0f %%", e->throttle * 100.f);
        snprintf(line[1], 48, "mix       %.2f", e->mix);
        snprintf(line[2], 48, "ignition  %s", e->ign ? "on" : "cut");
    } else if (tab == 1)
        snprintf(line[0], 48, "timing    %.0f deg BTDC", e->spark_deg);
    else if (tab == 2) {
        snprintf(line[0], 48, "atmo      %.0f kPa", e->atmo / 1000.f);
        snprintf(line[1], 48, "air temp  %.0f C", e->tamb - 273.f);
    } else {
        snprintf(line[0], 48, "comp      %.1f :1", e->cr);
        snprintf(line[1], 48, "bore      %.0f mm", e->bore * 1000.f);
    }
    for (i = 0; i < 4; i++) {
        if (!line[i][0])
            continue;
        if (i == item)
            fill(8, 60 + i * 14, 216, 13, hi);
        text(12, 62 + i * 14, line[i], wh);
    }
    text(12, 148, "A/D tabs  arrows  Esc", rgb565(140, 150, 170));
}
