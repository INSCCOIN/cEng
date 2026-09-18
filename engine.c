#include "engine.h"
#include <math.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#define PATM 101325.0
#define FUEL_MJ 44.0e6
#define FUEL_MM 0.11423
#define MOL_AFR 12.5
#define IFLY 0.18

static float wrap(float a, float t)
{
    while (a < 0)
        a += t;
    while (a >= t)
        a -= t;
    return a;
}

int eng_stroke_at(float th)
{
    float d = wrap(th, 4.f * (float)M_PI) * (180.f / (float)M_PI);
    if (d < 180)
        return 2;
    if (d < 360)
        return 3;
    if (d < 540)
        return 0;
    return 1;
}

void eng_piston(const Engine *e, double th, double *pinx, double *piny, double *pis)
{
    double s = sin(th), c = cos(th);
    *pinx = e->r * s;
    *piny = -e->r * c;
    *pis = e->r * c + sqrt(e->rod * e->rod - e->r * e->r * s * s);
}

double eng_vol(const Engine *e, double th)
{
    double s = sin(th), c = cos(th);
    double x = e->r * (1.0 - c) + (e->rod - sqrt(e->rod * e->rod - e->r * e->r * s * s));
    return e->vc + e->area * x;
}

void eng_rebuild(Engine *e)
{
    e->r = e->stroke * 0.5f;
    e->area = (float)M_PI * 0.25f * e->bore * e->bore;
    if (e->cr < 6.f)
        e->cr = 6.f;
    if (e->cr > 14.f)
        e->cr = 14.f;
    e->vc = (e->area * e->stroke) / (e->cr - 1.f);
}

void eng_init(Engine *e)
{
    int i;
    e->bore = 0.086f;
    e->stroke = 0.086f;
    e->rod = 0.145f;
    e->spark_deg = 12.f;
    e->atmo = (float)PATM;
    e->tamb = 293.f;
    e->mix = 1.0f;
    e->throttle = 0.35f;
    e->ign = 1;
    for (i = 0; i < NC; i++) {
        e->cyl[i].phase = (i & 1) ? M_PI : 0.0;
        e->cyl[i].fire = i * (M_PI);
    }
    eng_rebuild(e);
    eng_reset(e);
}

void eng_reset(Engine *e)
{
    int i;
    e->th = 0;
    e->w = 0;
    e->rpm = 0;
    e->starter = 0;
    e->autostart = 0;
    e->start_t = 0;
    e->work_acc = 0;
    e->fire_str = 0;
    e->exh_open = 0;
    gas_init(&e->man, e->atmo * (0.3 + 0.7 * e->throttle), 0.0035, e->tamb);
    gas_init(&e->exh, e->atmo, 0.006, e->tamb);
    for (i = 0; i < NC; i++) {
        double V = eng_vol(e, e->th + e->cyl[i].phase);
        gas_init(&e->cyl[i].ch, e->atmo, V > 1e-6 ? V : 1e-5, e->tamb);
        e->cyl[i].lit = e->cyl[i].fired = 0;
        e->cyl[i].travel_x = e->cyl[i].travel_y = 0;
        e->cyl[i].last_vol = V;
        e->cyl[i].flame_v = 15;
        e->cyl[i].burn_eff = 0.7;
    }
    e->p = (float)gas_P(&e->cyl[0].ch);
    e->t = (float)gas_T(&e->cyl[0].ch);
    e->fuel = 0;
}

static void ignite(Engine *e, Cyl *c)
{
    double afr, eq;
    if (c->lit)
        return;
    if (c->ch.p_fuel < 1e-5)
        return;
    afr = c->ch.p_o2 / c->ch.p_fuel;
    eq = afr / MOL_AFR;
    if (eq < 0.45 || eq > 2.1)
        return;
    c->lit = 1;
    c->fired = 1;
    c->travel_x = c->travel_y = 0;
    c->last_vol = c->ch.V;
    c->flame_v = 10.0 + 18.0 * (gas_T(&c->ch) / 700.0);
    if (c->flame_v > 45)
        c->flame_v = 45;
    c->burn_eff = 0.5 + 0.3 * e->mix;
    if (c->burn_eff > 0.92)
        c->burn_eff = 0.92;
    e->fire_str = 1.f;
}

static void burn(Cyl *c, double bore, double area, double dt)
{
    double vol, hx, hy, lx, ly, bv, pbv, litv, n, fm, mass;
    if (!c->lit)
        return;
    vol = c->ch.V;
    hx = bore * 0.5;
    hy = vol / (area + 1e-12);
    lx = c->travel_x;
    ly = c->travel_y * (vol / (c->last_vol > 1e-12 ? c->last_vol : vol));
    c->travel_x = lx + dt * c->flame_v;
    c->travel_y = ly + dt * c->flame_v;
    if (c->travel_x > hx)
        c->travel_x = hx;
    if (c->travel_y > hy)
        c->travel_y = hy;
    if (c->travel_x <= lx && c->travel_y <= ly) {
        c->lit = 0;
        c->last_vol = vol;
        return;
    }
    bv = c->travel_x * c->travel_x * GAS_PI * c->travel_y;
    pbv = lx * lx * GAS_PI * ly;
    litv = bv - pbv;
    if (litv < 0)
        litv = 0;
    n = (litv / (vol + 1e-12)) * c->ch.n;
    fm = n * c->ch.p_fuel * c->burn_eff;
    if (fm > c->ch.n * c->ch.p_fuel)
        fm = c->ch.n * c->ch.p_fuel;
    mass = fm * FUEL_MM;
    gas_add_energy(&c->ch, mass * FUEL_MJ);
    if (c->ch.n > 0) {
        c->ch.p_fuel -= fm / c->ch.n;
        c->ch.p_inert += fm / c->ch.n;
        if (c->ch.p_fuel < 0)
            c->ch.p_fuel = 0;
    }
    c->last_vol = vol;
}

void eng_step(Engine *e, float dt)
{
    int i, st;
    double t_gas = 0, t_fric, t_start, alpha;
    if (dt < 1e-4f)
        dt = 1e-4f;
    if (dt > 0.004f)
        dt = 0.004f;

    if (e->autostart) {
        e->ign = 1;
        e->start_t += dt;
        if (e->rpm < 1400.f && e->start_t < 4.f)
            e->starter = 1;
        else {
            e->starter = 0;
            e->autostart = 0;
        }
    }

    e->th = wrap(e->th + e->w * dt, 4.f * (float)M_PI);

    {
        double ptarget = e->atmo * (0.28 + 0.72 * e->throttle);
        double pm = gas_P(&e->man);
        e->man.Ek += (ptarget - pm) * e->man.V * (GAS_DOF * 0.5) * 12.0 * dt;
        if (e->man.Ek < 1)
            e->man.Ek = 1;
        e->man.p_fuel = 0.055 * e->mix * (0.4 + 0.6 * e->throttle);
        if (e->man.p_fuel > 0.12)
            e->man.p_fuel = 0.12;
        e->man.p_o2 = 0.21 * (1.0 - e->man.p_fuel);
        e->man.p_inert = 1.0 - e->man.p_fuel - e->man.p_o2;
    }

    e->exh_open = 0;
    for (i = 0; i < NC; i++) {
        Cyl *c = &e->cyl[i];
        double cth = e->th + (float)c->phase;
        double fth = wrap(e->th + (float)c->fire, 4.f * (float)M_PI);
        double s, P, tg;
        st = eng_stroke_at((float)fth);
        gas_set_volume(&c->ch, eng_vol(e, cth));

        if (st == 0) {
            gas_flow(&e->man, &c->ch, 3.5e-8, dt);
            c->ch.p_fuel = 0.7 * c->ch.p_fuel + 0.3 * e->man.p_fuel;
            c->ch.p_o2 = 0.7 * c->ch.p_o2 + 0.3 * e->man.p_o2;
            c->ch.p_inert = 1.0 - c->ch.p_fuel - c->ch.p_o2;
            c->fired = 0;
        }
        if (st == 3) {
            gas_flow(&c->ch, &e->exh, 2.2e-8, dt);
            e->exh_open = 1.f;
            e->exh.n = e->atmo * e->exh.V / (GAS_R * e->tamb);
            e->exh.Ek = e->tamb * (0.5 * GAS_DOF * e->exh.n * GAS_R);
        }

        {
            double four = 4.0 * M_PI;
            double spark = four - e->spark_deg * (M_PI / 180.0);
            double old = wrap((float)fth - e->w * dt, 4.f * (float)M_PI);
            if (e->ign && ((old < spark && fth >= spark) || (old > fth && fth < 0.25)))
                ignite(e, c);
        }
        burn(c, e->bore, e->area, dt);

        s = sin(cth);
        P = gas_P(&c->ch);
        tg = (P - e->atmo) * e->area * e->r * s * (1.0 + (e->r / e->rod) * cos(cth));
        if (e->starter && e->rpm < 500.f)
            tg *= 0.2;
        t_gas += tg;
    }

    t_fric = -0.008 * e->w - (e->w > 0 ? 0.25 : 0);
    t_start = e->starter ? 28.0 : 0;
    alpha = (t_gas + t_fric + t_start) / IFLY;
    e->w += (float)(alpha * dt);
    if (e->w < 0)
        e->w = 0;
    if (e->w > 850.f)
        e->w = 850.f;
    e->rpm = e->w * 60.f / (2.f * (float)M_PI);
    e->p = (float)gas_P(&e->cyl[0].ch);
    e->t = (float)gas_T(&e->cyl[0].ch);
    e->fuel = (float)e->cyl[0].ch.p_fuel;
    e->fire_str *= expf(-dt * 18.f);
    e->work_acc += (float)(t_gas * e->w * dt);
}
