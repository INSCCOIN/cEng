#include "gas.h"
#include <math.h>

void gas_init(Gas *g, double P, double V, double T)
{
    if (T < 50)
        T = 50;
    if (V < 1e-8)
        V = 1e-8;
    g->V = V;
    g->n = P * V / (GAS_R * T);
    if (g->n < 1e-9)
        g->n = 1e-9;
    g->Ek = T * (0.5 * GAS_DOF * g->n * GAS_R);
    g->p_fuel = 0.0;
    g->p_o2 = 0.21;
    g->p_inert = 0.79;
}

double gas_P(const Gas *g)
{
    if (g->V <= 0)
        return 0;
    return (2.0 / GAS_DOF) * g->Ek / g->V;
}

double gas_T(const Gas *g)
{
    double den = 0.5 * GAS_DOF * g->n * GAS_R;
    if (den <= 0)
        return 300;
    return g->Ek / den;
}

void gas_set_volume(Gas *g, double Vnew)
{
    double dV, P;
    if (Vnew < 1e-8)
        Vnew = 1e-8;
    P = gas_P(g);
    dV = Vnew - g->V;
    /* engine-sim: expansion does work, Ek falls (W = -P dV) */
    g->Ek += -P * dV;
    if (g->Ek < 1.0)
        g->Ek = 1.0;
    g->V = Vnew;
}

void gas_add_energy(Gas *g, double J)
{
    g->Ek += J;
    if (g->Ek < 1.0)
        g->Ek = 1.0;
}

static void norm_mix(Gas *g)
{
    double s = g->p_fuel + g->p_o2 + g->p_inert;
    if (s < 1e-12)
        return;
    g->p_fuel /= s;
    g->p_o2 /= s;
    g->p_inert /= s;
}

double gas_flow(Gas *hi, Gas *lo, double k, double dt)
{
    double Ph = gas_P(hi), Pl = gas_P(lo);
    double dn, frac;
    if (Ph < Pl) {
        Gas *t = hi;
        hi = lo;
        lo = t;
        Ph = gas_P(hi);
        Pl = gas_P(lo);
    }
    dn = k * (Ph - Pl) * dt;
    if (dn > hi->n * 0.25)
        dn = hi->n * 0.25;
    if (dn < 0)
        dn = 0;
    if (hi->n - dn < 1e-9)
        return 0;
    frac = dn / hi->n;
    lo->n += dn;
    hi->n -= dn;
    lo->Ek += hi->Ek * frac;
    hi->Ek -= hi->Ek * frac;
    lo->p_fuel += (hi->p_fuel - lo->p_fuel) * frac;
    lo->p_o2 += (hi->p_o2 - lo->p_o2) * frac;
    lo->p_inert += (hi->p_inert - lo->p_inert) * frac;
    norm_mix(hi);
    norm_mix(lo);
    if (hi->Ek < 1)
        hi->Ek = 1;
    if (lo->Ek < 1)
        lo->Ek = 1;
    return dn;
}
