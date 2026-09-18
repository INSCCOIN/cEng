#ifndef CENG_GAS_H
#define CENG_GAS_H

/* Ideal-gas state matching Ange Yaghi engine-sim GasSystem (MIT).
 * n moles, V m^3, Ek translational energy J.
 * P = (2/dof) Ek / V    T = Ek / (0.5 dof n R)
 */

#define GAS_R 8.31446261815324
#define GAS_DOF 5.0
#define GAS_PI 3.14159265358979323846

typedef struct {
    double n;
    double V;
    double Ek;
    double p_fuel, p_o2, p_inert; /* mix, sum ~ 1 */
} Gas;

void gas_init(Gas *g, double P, double V, double T);
double gas_P(const Gas *g);
double gas_T(const Gas *g);
void gas_set_volume(Gas *g, double Vnew);
void gas_add_energy(Gas *g, double J);
double gas_flow(Gas *a, Gas *b, double k, double dt);

#endif
