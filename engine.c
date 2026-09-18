#include "engine.h"
#include <math.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#define PATM 101325.f
#define GAMMA 1.30f
#define IFLY 0.08f

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
    float d = e->th * (180.f / (float)M_PI); /* 0..720 */
    if (d < 180)
        return 2; /* power */
    if (d < 360)
        return 3; /* exhaust */
    if (d < 540)
        return 0; /* intake */
    return 1;     /* compression */
}

static float volume(const Engine *e)
{
    float s = sinf(e->th), c = cosf(e->th);
    float x = e->r * (1.f - c) + (e->rod - sqrtf(e->rod * e->rod - e->r * e->r * s * s));
    return e->vc + e->area * x;
}

void eng_init(Engine *e)
{
    e->bore = 0.086f;
    e->stroke = 0.086f;
    e->rod = 0.136f;
    e->r = e->stroke * 0.5f;
    e->area = (float)M_PI * 0.25f * e->bore * e->bore;
    e->cr = 9.0f;
    e->vc = (e->area * e->stroke) / (e->cr - 1.f);
    eng_reset(e);
}

void eng_reset(Engine *e)
{
    e->th = 3.2f * (float)M_PI; /* late compression, ready to fire */
    e->w = 0;
    e->p = PATM;
    e->t = 350.f;
    e->fuel = 0;
    e->throttle = 0.25f;
    e->rpm = 0;
    e->ign = 1;
    e->starter = 0;
    e->fired = 0;
    e->work_acc = 0;
    e->fire_str = 0;
    e->exh_open = 0;
}

void eng_step(Engine *e, float dt)
{
    float v0, v1, t_gas, t_fric, t_start, alpha;
    int st0, st1;
    if (dt < 1e-4f)
        dt = 1e-4f;
    if (dt > 0.005f)
        dt = 0.005f;

    st0 = eng_stroke(e);
    v0 = volume(e);

    /* crank */
    e->th = wrap4pi(e->th + e->w * dt);
    st1 = eng_stroke(e);
    v1 = volume(e);
    if (v1 < e->vc * 1.02f)
        v1 = e->vc * 1.02f;

    /* compression / expansion */
    e->p *= powf(v0 / v1, GAMMA);
    if (e->p < 20000.f)
        e->p = 20000.f;
    if (e->p > 8e6f)
        e->p = 8e6f;

    if (st1 == 0) { /* intake */
        float pin = 28000.f + e->throttle * 74000.f;
        e->p += (pin - e->p) * (1.f - expf(-dt * 40.f));
        e->fuel += (e->throttle - e->fuel) * (1.f - expf(-dt * 20.f));
        e->fired = 0;
    } else if (st1 == 3) { /* exhaust */
        e->p += (PATM - e->p) * (1.f - expf(-dt * 35.f));
        e->fuel *= expf(-dt * 8.f);
        e->exh_open = 1.f;
    } else
        e->exh_open *= expf(-dt * 6.f);

    /* spark ~5 deg BTDC into power (theta wrap 4pi -> 0) */
    if (e->ign && !e->fired && st0 == 1 && st1 == 2 && e->fuel > 0.08f && e->p > 180000.f) {
        float q = 480000.f * e->fuel; /* J/m^3-ish scaled into Pa */
        e->p += q;
        e->fired = 1;
        e->fire_str = 1.f;
        e->fuel *= 0.15f;
    }
    e->fire_str *= expf(-dt * 25.f);

    /* torque from gas: F * r * sin(th) * rod factor */
    {
        float s = sinf(e->th);
        float tanf_ = s / (sqrtf(e->rod * e->rod - e->r * e->r * s * s) / e->rod + 1e-4f);
        t_gas = (e->p - PATM) * e->area * e->r * tanf_;
        /* slider-crank exact-enough: F * r * sin * (1 + (r/L)cos) */
        t_gas = (e->p - PATM) * e->area * e->r * s * (1.f + (e->r / e->rod) * cosf(e->th));
    }
    t_fric = -0.012f * e->w - (e->w > 0 ? 0.35f : (e->w < 0 ? -0.35f : 0));
    t_start = 0;
    if (e->starter && e->rpm < 600.f)
        t_start = 9.0f;
    alpha = (t_gas + t_fric + t_start) / IFLY;
    e->w += alpha * dt;
    if (e->w < -20.f)
        e->w = -20.f;
    if (e->w > 900.f)
        e->w = 900.f; /* ~8600 rpm cap */
    e->rpm = e->w * 60.f / (2.f * (float)M_PI);
    if (e->rpm < 0)
        e->rpm = 0;
    e->work_acc += t_gas * e->w * dt;
}
