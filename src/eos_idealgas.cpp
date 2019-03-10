// Copyright 2018 @ Chun Shen
#include "eos_idealgas.h"

#include <cmath>

EOS_idealgas::EOS_idealgas() {
    set_EOS_id(0);
    set_number_of_tables(0);
    set_eps_max(1e5);
    Nc = 3.;
    Nf = 2.5;
    set_flag_muB(false);
    set_flag_muS(false);
    set_flag_muC(false);
}

void EOS_idealgas::initialize_eos() {
    music_message.info("initialze EOS ideal gas ...");
}

double EOS_idealgas::get_temperature(double eps, double rhob) const {
    const double factor = (M_PI*M_PI)/90.*(2*(Nc*Nc-1)+7./2*Nc*Nf);
    return pow((eps/3.0/factor), .25);
}

double EOS_idealgas::get_s2e(double s, double rhob) const {
    const double factor = (M_PI*M_PI)/90.*(2*(Nc*Nc-1)+7./2*Nc*Nf);
    return(3./4.*s*pow(3.*s/4./(3.0*factor), 1./3.));    // in 1/fm^4
}

double EOS_idealgas::get_dedT(double eps, double rhob) const {
    const double factor = (M_PI*M_PI)/90.*(2*(Nc*Nc-1)+7./2*Nc*Nf);
    const double dedT = 12.*factor*pow(eps/(3.*factor), 0.75);
    return(dedT);
}

double EOS_idealgas::get_correlation_length(
                            const double eps, const double rhob) const {
    const double T_local = get_temperature(eps, rhob);
    const double chi_B   = get_chi_B(eps, rhob);
    const double C_xi    = 1.0;
    const double xi      = std::max(C_xi/T_local, sqrt(chi_B/C_xi));
    return(xi);
}
