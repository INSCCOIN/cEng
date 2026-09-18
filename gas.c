#include "gas.h"
#include <math.h>

static void clamp_state(Gas *g)
{
    double T, P;
    if (g->V < 1e-8)
        g->V = 1e-8;
    if (g->n < 1e-8)
        g->n = 1e-8;
    T = g->Ek / (0.5 * GAS_DOF * g->n * GAS_R);
    if (T < 250.0) {
        T = 250.0;
        g->Ek = T * (0.5 * GAS_DOF * g->n * GAS_R);
    }
    if (T > 3500.0) {
        T = 3500.0;
        g->Ek = T * (0.5 * GAS_DOF * g->n * GAS_R);
    }
    P = (2.0 / GAS_DOF) * g->Ek / g->V;
    if (P < 25000.0) {
        g->Ek = 25000.0 * g->V * GAS_DOF / 2.0;
    }
}

void gas_init(Gas *g, double P, double V, double T)
{
    if (T < 250)
        T = 250;
    if (V < 1e-8)
        V = 1e-8;
    if (P < 25000)
        P = 25000;
    g->V = V;
    g->n = P * V / (GAS_R * T);
    if (g->n < 1e-8)
        g->n = 1e-8;
    g->Ek = T * (0.5 * GAS_DOF * g->n * GAS_R);
    g->p_fuel = 0.0;
    g->p_o2 = 0.21;
    g->p_inert = 0.79;
}

double gas_P(const Gas *g)
{
    if (g->V <= 0)
        return 25000;
    return (2.0 / GAS_DOF) * g->Ek / g->V;
}

double gas_T(const Gas *g)
{
    double den = 0.5 * GAS_DOF * g->n * GAS_R;
    if (den <= 0)
        return 293;
    return g->Ek / den;
}

void gas_set_volume(Gas *g, double Vnew)
{
    double dV, P;
    if (Vnew < 1e-8)
        Vnew = 1e-8;
    P = gas_P(g);
    dV = Vnew - g->V;
    g->Ek += -P * dV;
    g->V = Vnew;
    clamp_state(g);
}

void gas_add_energy(Gas *g, double J)
{
    g->Ek += J;
    clamp_state(g);
}

static void norm_mix(Gas *g)
{
    double s = g->p_fuel + g->p_o2 + g->p_inert;
    if (s < 1e-12) {
        g->p_o2 = 0.21;
        g->p_inert = 0.79;
        g->p_fuel = 0;
        return;
    }
    g->p_fuel /= s;
    g->p_o2 /= s;
    g->p_inert /= s;
}

double gas_flow(Gas *a, Gas *b, double k, double dt)
{
    double Pa = gas_P(a), Pb = gas_P(b);
    Gas *src = Pa >= Pb ? a : b;
    Gas *dst = Pa >= Pb ? b : a;
    double dn, frac;
    dn = k * fabs(Pa - Pb) * dt;
    if (dn > src->n * 0.2)
        dn = src->n * 0.2;
    if (dn < 0)
        return 0;
    if (src->n <= 1e-8)
        return 0;
    frac = dn / src->n;
    dst->n += dn;
    src->n -= dn;
    dst->Ek += src->Ek * frac;
    src->Ek -= src->Ek * frac;
    dst->p_fuel = dst->p_fuel * (1.0 - frac) + src->p_fuel * frac;
    dst->p_o2 = dst->p_o2 * (1.0 - frac) + src->p_o2 * frac;
    dst->p_inert = dst->p_inert * (1.0 - frac) + src->p_inert * frac;
    norm_mix(src);
    norm_mix(dst);
    clamp_state(src);
    clamp_state(dst);
    return dn;
}
