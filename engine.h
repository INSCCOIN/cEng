#ifndef CENG_H
#define CENG_H
#include "gas.h"

typedef struct {
    /* geometry (SI) */
    float bore, stroke, rod, r, area, vc, cr;
    /* state */
    float th;      /* crank rad, 0 = TDC combustion */
    float w;       /* rad/s */
    float p;       /* chamber Pa */
    float t;       /* gas K */
    float fuel;    /* 0-1 mix in chamber */
    float throttle; /* 0-1 */
    float rpm;
    int ign;
    int starter;
    int fired;     /* this cycle */
    int cycle;     /* 0..3  intake comp power exh  based on 0-4pi */
    float work_acc;
    float fire_str; /* audio */
    float exh_open;
    float spark_deg; /* BTDC */
    float atmo;      /* Pa */
    float tamb;      /* K ambient */
    float mix;       /* fuel richness 0.5-1.5 */
    int autostart;
    Gas ch, man, exh; /* chamber, intake manifold, exhaust dump */
    int lit;
    double travel_x, travel_y, last_vol, flame_v, burn_eff;
} Engine;

void eng_init(Engine *e);
void eng_reset(Engine *e);
void eng_rebuild(Engine *e);
void eng_step(Engine *e, float dt);
int eng_stroke(const Engine *e); /* 0 in 1 comp 2 pwr 3 exh */

#endif
