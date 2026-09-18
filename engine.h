#ifndef CENG_H
#define CENG_H
#include "gas.h"

enum { NC = 4 };

typedef struct {
    Gas ch;
    int lit, fired;
    double travel_x, travel_y, last_vol, flame_v, burn_eff;
    double phase;
    double fire;
} Cyl;

typedef struct {
    float bore, stroke, rod, r, area, vc, cr;
    float th, w, rpm, throttle, spark_deg, atmo, tamb, mix;
    float p, t, fuel, fire_str, exh_open, work_acc;
    int ign, starter, autostart;
    float start_t;
    Cyl cyl[NC];
    Gas man, exh;
} Engine;

void eng_init(Engine *e);
void eng_reset(Engine *e);
void eng_rebuild(Engine *e);
void eng_step(Engine *e, float dt);
int eng_stroke_at(float th);
double eng_vol(const Engine *e, double th);
void eng_piston(const Engine *e, double th, double *pinx, double *piny, double *pis);

#endif
