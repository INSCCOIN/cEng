#include "engine.h"
#include <math.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#define PATM 101325.0
#define FUEL_MJ 44.0e6
#define FUEL_MOLMASS 0.11423
#define MOL_AFR 12.5

static float wrap4pi(float a)
{
    const float t = 4.f * (float)M_PI;
    while (a < 0)
        a += t;
    while (a >= t)
        a -= t;
    return a;
}

int eng_stroke(const Engine *e)
{
    float d = e->th * (180.f / (float)M_PI);
    if (d < 180)
        return 2;
    if (d < 360)
        return 3;
    if (d < 540)
        return 0;
    return 1;
}

static double chamber_volume(const Engine *e)
{
    double s = sin((double)e->th), c = cos((double)e->th);
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
    e->bore = 0.086f;
    e->stroke = 0.086f;
    e->rod = 0.136f;
    e->spark_deg = 8.f;
    e->atmo = (float)PATM;
    e->tamb = 293.f;
    e->mix = 1.f;
    e->throttle = 0.25f;
    e->ign = 1;
    eng_rebuild(e);
    eng_reset(e);
}

void eng_reset(Engine *e)
{
    double V;
    e->th = 3.2f * (float)M_PI;
    e->w = 0;
    e->rpm = 0;
    e->starter = 0;
    e->autostart = 0;
    e->fired = 0;
    e->work_acc = 0;
    e->fire_str = 0;
    e->exh_open = 0;
    e->lit = 0;
    e->travel_x = e->travel_y = 0;
    e->flame_v = 12.0;
    e->burn_eff = 0.75;
    V = chamber_volume(e);
    gas_init(&e->ch, e->atmo, V > 1e-6 ? V : 1e-5, e->tamb);
    gas_init(&e->man, e->atmo * 0.4, 0.002, e->tamb);
    gas_init(&e->exh, e->atmo, 0.004, e->tamb);
    e->p = (float)gas_P(&e->ch);
    e->t = (float)gas_T(&e->ch);
    e->fuel = 0;
}

static void ignite_ange(Engine *e)
{
    double afr, eq;
    if (e->lit)
        return;
    if (e->ch.p_fuel <= 1e-6)
        return;
    afr = e->ch.p_o2 / e->ch.p_fuel;
    eq = afr / MOL_AFR;
    if (eq < 0.5 || eq > 1.9)
        return;
    e->lit = 1;
    e->fired = 1;
    e->travel_x = 0;
    e->travel_y = 0;
    e->last_vol = e->ch.V;
    e->flame_v = 8.0 + 20.0 * (gas_T(&e->ch) / 600.0);
    if (e->flame_v > 40)
        e->flame_v = 40;
    e->burn_eff = 0.55 + 0.25 * e->mix;
    if (e->burn_eff > 0.9)
        e->burn_eff = 0.9;
    e->fire_str = 1.f;
}

static void burn_ange(Engine *e, double dt)
{
    double vol = e->ch.V;
    double hx = e->bore * 0.5;
    double hy = vol / ((double)e->area + 1e-12);
    double lx, ly, bv, pbv, litv, n, fuel_mol, mass;
    if (!e->lit)
        return;
    lx = e->travel_x;
    ly = e->travel_y * (vol / (e->last_vol > 1e-12 ? e->last_vol : vol));
    e->travel_x = lx + dt * e->flame_v;
    e->travel_y = ly + dt * e->flame_v;
    if (e->travel_x > hx)
        e->travel_x = hx;
    if (e->travel_y > hy)
        e->travel_y = hy;
    if (e->travel_x <= lx && e->travel_y <= ly) {
        e->lit = 0;
        e->last_vol = vol;
        return;
    }
    bv = e->travel_x * e->travel_x * GAS_PI * e->travel_y;
    pbv = lx * lx * GAS_PI * ly;
    litv = bv - pbv;
    if (litv < 0)
        litv = 0;
    n = (litv / vol) * e->ch.n;
    fuel_mol = n * e->ch.p_fuel * e->burn_eff;
    if (fuel_mol > e->ch.n * e->ch.p_fuel)
        fuel_mol = e->ch.n * e->ch.p_fuel;
    mass = fuel_mol * FUEL_MOLMASS;
    gas_add_energy(&e->ch, mass * FUEL_MJ);
    e->ch.p_fuel -= (e->ch.n > 0) ? fuel_mol / e->ch.n : 0;
    if (e->ch.p_fuel < 0)
        e->ch.p_fuel = 0;
    e->ch.p_inert += (e->ch.n > 0) ? fuel_mol / e->ch.n : 0;
    e->last_vol = vol;
}

void eng_step(Engine *e, float dt)
{
    double t_gas, t_fric, t_start, alpha;
    int st;
    double four, spark, th_old;
    if (dt < 1e-4f)
        dt = 1e-4f;
    if (dt > 0.005f)
        dt = 0.005f;

    e->th = wrap4pi(e->th + e->w * dt);
    st = eng_stroke(e);
    gas_set_volume(&e->ch, chamber_volume(e));

    {
        double ptarget = e->atmo * (0.25 + 0.75 * e->throttle);
        double pm = gas_P(&e->man);
        e->man.Ek += (ptarget - pm) * e->man.V * (GAS_DOF * 0.5) * 8.0 * dt;
        if (e->man.Ek < 1)
            e->man.Ek = 1;
        e->man.p_fuel = 0.06 * e->mix * e->throttle;
        e->man.p_o2 = 0.21 * (1.0 - e->man.p_fuel);
        e->man.p_inert = 1.0 - e->man.p_fuel - e->man.p_o2;
    }

    if (st == 0)
        gas_flow(&e->man, &e->ch, 1.2e-8, dt);
    if (st == 3) {
        gas_flow(&e->ch, &e->exh, 1.4e-8, dt);
        e->exh_open = 1.f;
        e->exh.n = e->atmo * e->exh.V / (GAS_R * e->tamb);
        e->exh.Ek = e->tamb * (0.5 * GAS_DOF * e->exh.n * GAS_R);
        e->exh.p_fuel = 0;
        e->exh.p_o2 = 0.21;
        e->exh.p_inert = 0.79;
    } else
        e->exh_open *= expf(-dt * 6.f);

    four = 4.0 * M_PI;
    spark = four - e->spark_deg * (M_PI / 180.0);
    th_old = wrap4pi(e->th - e->w * dt);
    if (e->ign && ((th_old < spark && e->th >= spark) || (th_old > e->th && e->th < 0.2f)))
        ignite_ange(e);

    if (st == 0)
        e->fired = 0;

    burn_ange(e, dt);
    e->fire_str *= expf(-dt * 25.f);

    {
        double s = sin((double)e->th);
        double P = gas_P(&e->ch);
        t_gas = (P - e->atmo) * e->area * e->r * s * (1.0 + (e->r / e->rod) * cos((double)e->th));
    }
    t_fric = -0.012 * e->w - (e->w > 0 ? 0.35 : (e->w < 0 ? -0.35 : 0));
    t_start = (e->starter && e->rpm < 600.f) ? 9.0 : 0;
    alpha = (t_gas + t_fric + t_start) / 0.08;
    e->w += (float)(alpha * dt);
    if (e->w < -20.f)
        e->w = -20.f;
    if (e->w > 900.f)
        e->w = 900.f;
    e->rpm = e->w * 60.f / (2.f * (float)M_PI);
    if (e->rpm < 0)
        e->rpm = 0;
    e->p = (float)gas_P(&e->ch);
    e->t = (float)gas_T(&e->ch);
    e->fuel = (float)e->ch.p_fuel;
    e->work_acc += (float)(t_gas * e->w * dt);
}
