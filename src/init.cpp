// Copyright 2011 @ Bjoern Schenke, Sangyong Jeon, and Charles Gale
#include <string>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <omp.h>
#include "./util.h"
#include "./cell.h"
#include "./grid.h"
#include "./init.h"
#include "./eos.h"

#ifndef _OPENMP
    #define omp_get_thread_num() 0
#endif

using std::vector;
using std::ifstream;

Init::Init(const EOS &eosIn, InitData &DATA_in, hydro_source &hydro_source_in) :
    DATA(DATA_in), eos(eosIn) , hydro_source_terms(hydro_source_in) {}

void Init::InitArena(SCGrid &arena_prev, SCGrid &arena_current,
                     SCGrid &arena_future) {
    music_message.info("initArena");
    if (DATA.Initial_profile == 0) {
        music_message << "Using Initial_profile=" << DATA.Initial_profile;
        music_message << "nx=" << DATA.nx << ", ny=" << DATA.ny;
        music_message << "dx=" << DATA.delta_x << ", dy=" << DATA.delta_y;
        music_message.flush("info");
    } else if (DATA.Initial_profile == 1) {
        music_message << "Using Initial_profile=" << DATA.Initial_profile;
        DATA.nx = 2;
        DATA.ny = 2;
        DATA.neta = 695;
        DATA.delta_x = 0.1;
        DATA.delta_y = 0.1;
        DATA.delta_eta = 0.02;
        music_message << "nx=" << DATA.nx << ", ny=" << DATA.ny;
        music_message << "dx=" << DATA.delta_x << ", dy=" << DATA.delta_y;
        music_message << "neta=" << DATA.neta << ", deta=" << DATA.delta_eta;
        music_message.flush("info");
    } else if (DATA.Initial_profile == 8) {
        music_message.info(DATA.initName);
        ifstream profile(DATA.initName.c_str());
        std::string dummy;
        int nx, ny, neta;
        double deta, dx, dy, dummy2;
        // read the first line with general info
        profile >> dummy >> dummy >> dummy2
                >> dummy >> neta >> dummy >> nx >> dummy >> ny
                >> dummy >> deta >> dummy >> dx >> dummy >> dy;
        profile.close();
        music_message << "Using Initial_profile=" << DATA.Initial_profile
                      << ". Overwriting lattice dimensions:";
        DATA.nx = nx;
        DATA.ny = ny;
        DATA.delta_x = dx;
        DATA.delta_y = dy;

        music_message << "neta=" << neta << ", nx=" << nx << ", ny=" << ny;
        music_message << "deta=" << DATA.delta_eta << ", dx=" << DATA.delta_x
                      << ", dy=" << DATA.delta_y;
        music_message.flush("info");
    } else if (   DATA.Initial_profile == 9 || DATA.Initial_profile == 91
               || DATA.Initial_profile == 92) {
        music_message.info(DATA.initName);
        ifstream profile(DATA.initName.c_str());
        std::string dummy;
        int nx, ny, neta;
        double deta, dx, dy, dummy2;
        // read the first line with general info
        profile >> dummy >> dummy >> dummy2
                >> dummy >> neta >> dummy >> nx >> dummy >> ny
                >> dummy >> deta >> dummy >> dx >> dummy >> dy;
        profile.close();
        music_message << "Using Initial_profile=" << DATA.Initial_profile
                      << ". Overwriting lattice dimensions:";
        DATA.nx = nx;
        DATA.ny = ny;
        DATA.neta = neta;
        DATA.delta_x = dx;
        DATA.delta_y = dy;
        DATA.delta_eta = 0.1;

        music_message << "neta=" << neta << ", nx=" << nx << ", ny=" << ny;
        music_message << "deta=" << DATA.delta_eta << ", dx=" << DATA.delta_x
                      << ", dy=" << DATA.delta_y;
        music_message.flush("info");
    } else if (DATA.Initial_profile == 13) {
        DATA.tau0 = hydro_source_terms.get_source_tau_min() - DATA.delta_tau;
        DATA.tau0 = std::max(0.1, DATA.tau0);
    } else if (DATA.Initial_profile == 30) {
        DATA.tau0 = hydro_source_terms.get_source_tau_min();
    } else if (DATA.Initial_profile == 42) {
        // initial condition from the JETSCAPE framework
        music_message << "Using Initial_profile=" << DATA.Initial_profile 
                      << ". Overwriting lattice dimensions:";
        music_message.flush("info");

        const int nx = static_cast<int>(
                sqrt(jetscape_initial_energy_density.size()/DATA.neta));
        const int ny = nx;
        DATA.nx = nx;
        DATA.ny = ny;
        DATA.x_size = DATA.delta_x*(nx - 1);
        DATA.y_size = DATA.delta_y*(ny - 1);

        music_message << "neta = " << DATA.neta
                      << ", nx = " << nx << ", ny = " << ny;
        music_message.flush("info");
        music_message << "deta=" << DATA.delta_eta
                      << ", dx=" << DATA.delta_x 
                      << ", dy=" << DATA.delta_y;
        music_message.flush("info");
        music_message << "x_size = "     << DATA.x_size
                      << ", y_size = "   << DATA.y_size
                      << ", eta_size = " << DATA.eta_size;
        music_message.flush("info");
    } else if (DATA.Initial_profile == 101) {
        music_message << "Using Initial_profile=" << DATA.Initial_profile;
        music_message << "nx=" << DATA.nx << ", ny=" << DATA.ny;
        music_message << "dx=" << DATA.delta_x << ", dy=" << DATA.delta_y;
        music_message.flush("info");
    }

    // initialize arena
    arena_prev    = SCGrid(DATA.nx, DATA.ny, DATA.neta);
    arena_current = SCGrid(DATA.nx, DATA.ny, DATA.neta);
    arena_future  = SCGrid(DATA.nx, DATA.ny, DATA.neta);
    music_message.info("Grid allocated.");

    InitTJb(arena_prev, arena_current);

    if (DATA.output_initial_density_profiles == 1) {
        output_initial_density_profiles(arena_current);
    }
}/* InitArena */

//! This is a shell function to initial hydrodynamic fields
void Init::InitTJb(SCGrid &arena_prev, SCGrid &arena_current) {
    if (DATA.Initial_profile == 0) {
        // Gubser flow test
        music_message.info(" Perform Gubser flow test ... ");
        music_message.info(" ----- information on initial distribution -----");
        
        #pragma omp parallel for
        for (int ieta = 0; ieta < arena_current.nEta(); ieta++) {
            initial_Gubser_XY(ieta, arena_prev, arena_current);
        }
    } else if (DATA.Initial_profile == 1) {
        // code test in 1+1 D vs Monnai's results
        music_message.info(" Perform 1+1D test vs Monnai's results... ");
        initial_1p1D_eta(arena_prev, arena_current);
    } else if (DATA.Initial_profile == 8) {
        // read in the profile from file
        // - IPGlasma initial conditions with initial flow
        music_message.info(" ----- information on initial distribution -----");
        music_message << "file name used: " << DATA.initName;
        music_message.flush("info");
  
        #pragma omp parallel for
        for (int ieta = 0; ieta < arena_current.nEta(); ieta++) {
            initial_IPGlasma_XY(ieta, arena_prev, arena_current);
        }
    } else if (   DATA.Initial_profile == 9 || DATA.Initial_profile == 91
               || DATA.Initial_profile == 92) {
        // read in the profile from file
        // - IPGlasma initial conditions with initial flow
        // and initial shear viscous tensor
        music_message.info(" ----- information on initial distribution -----");
        music_message << "file name used: " << DATA.initName;
        music_message.flush("info");
  
        #pragma omp parallel for
        for (int ieta = 0; ieta < arena_current.nEta(); ieta++) {
            initial_IPGlasma_XY_with_pi(ieta, arena_prev, arena_current);
        }
    } else if (DATA.Initial_profile == 11) {
        // read in the transverse profile from file with finite rho_B
        // the initial entropy and net baryon density profile are
        // constructed by nuclear thickness function TA and TB.
        // Along the longitudinal direction an asymmetric contribution from
        // target and projectile thickness function is allowed
        music_message.info(" ----- information on initial distribution -----");
        music_message << "file name used: " << DATA.initName_TA << " and "
                      << DATA.initName_TB;
        music_message.flush("info");

        #pragma omp parallel for
        for (int ieta = 0; ieta < arena_current.nEta(); ieta++) {
            initial_MCGlb_with_rhob_XY(ieta, arena_prev, arena_current);
        }
    } else if (DATA.Initial_profile == 13) {
        music_message.info("Initialize hydro with source terms");
        #pragma omp parallel for
        for (int ieta = 0; ieta < arena_current.nEta(); ieta++) {
            initial_MCGlbLEXUS_with_rhob_XY(ieta, arena_prev, arena_current);
        }
    } else if (DATA.Initial_profile == 30) {
        #pragma omp parallel for
        for (int ieta = 0; ieta < arena_current.nEta(); ieta++) {
            initial_AMPT_XY(ieta, arena_prev, arena_current);
        }
    } else if (DATA.Initial_profile == 42) {
        // initialize hydro with vectors from JETSCAPE
        music_message.info(" ----- information on initial distribution -----");
        music_message << "initialized with a JETSCAPE initial condition.";
        music_message.flush("info");
        #pragma omp parallel for
        for (int ieta = 0; ieta < arena_current.nEta(); ieta++) {
            initial_with_jetscape(ieta, arena_prev, arena_current);
        }
        clean_up_jetscape_arrays();
    } else if (DATA.Initial_profile == 101) {
        initial_UMN_with_rhob(arena_prev, arena_current);
    }
    music_message.info("initial distribution done.");
}

void Init::initial_Gubser_XY(int ieta, SCGrid &arena_prev,
                             SCGrid &arena_current) {
    std::string input_filename;
    std::string input_filename_prev;
    if (DATA.turn_on_shear == 1) {
        input_filename = "tests/Gubser_flow/Initial_Profile.dat";
    } else {
        input_filename = "tests/Gubser_flow/y=0_tau=1.00_ideal.dat";
        input_filename_prev = "tests/Gubser_flow/y=0_tau=0.98_ideal.dat";
    }
    
    ifstream profile(input_filename.c_str());
    if (!profile.good()) {
        music_message << "Init::InitTJb: "
                      << "Can not open the initial file: " << input_filename;
        music_message.flush("error");
        exit(1);
    }
    ifstream profile_prev;
    if (DATA.turn_on_shear == 0) {
        profile_prev.open(input_filename_prev.c_str());
        if (!profile_prev.good()) {
            music_message << "Init::InitTJb: "
                          << "Can not open the initial file: "
                          << input_filename_prev;
            music_message.flush("error");
            exit(1);
        }
    }

    const int nx = arena_current.nX();
    const int ny = arena_current.nY();
    double temp_profile_ed[nx][ny];
    double temp_profile_ux[nx][ny];
    double temp_profile_uy[nx][ny];
    double temp_profile_ed_prev[nx][ny];
    double temp_profile_rhob[nx][ny];
    double temp_profile_rhob_prev[nx][ny];
    double temp_profile_ux_prev[nx][ny];
    double temp_profile_uy_prev[nx][ny];
    double temp_profile_pixx[nx][ny];
    double temp_profile_piyy[nx][ny];
    double temp_profile_pixy[nx][ny];
    double temp_profile_pi00[nx][ny];
    double temp_profile_pi0x[nx][ny];
    double temp_profile_pi0y[nx][ny];
    double temp_profile_pi33[nx][ny];

    double dummy;
    for (int ix = 0; ix < nx; ix++) {
        for (int iy = 0; iy < ny; iy++) {
            if (DATA.turn_on_shear == 1) {
                profile >> dummy >> dummy >> temp_profile_ed[ix][iy]
                        >> temp_profile_ux[ix][iy] >> temp_profile_uy[ix][iy];
                profile >> temp_profile_pixx[ix][iy]
                        >> temp_profile_piyy[ix][iy]
                        >> temp_profile_pixy[ix][iy]
                        >> temp_profile_pi00[ix][iy]
                        >> temp_profile_pi0x[ix][iy]
                        >> temp_profile_pi0y[ix][iy]
                        >> temp_profile_pi33[ix][iy];
            } else {
                profile >> dummy >> dummy >> temp_profile_ed[ix][iy]
                        >> temp_profile_rhob[ix][iy]
                        >> temp_profile_ux[ix][iy] >> temp_profile_uy[ix][iy];
                profile_prev >> dummy >> dummy >> temp_profile_ed_prev[ix][iy]
                             >> temp_profile_rhob_prev[ix][iy]
                             >> temp_profile_ux_prev[ix][iy]
                             >> temp_profile_uy_prev[ix][iy];
            }
        }
    }
    profile.close();
    if (DATA.turn_on_shear == 0) {
        profile_prev.close();
    }

    for (int ix = 0; ix < nx; ix++) {
        for (int iy = 0; iy< ny; iy++) {
            double rhob = 0.0;
            if (DATA.turn_on_shear == 0 && DATA.turn_on_rhob == 1) {
                rhob = temp_profile_rhob[ix][iy];
            }

            double epsilon = temp_profile_ed[ix][iy];
            
            arena_current(ix, iy, ieta).epsilon = epsilon;
            arena_prev   (ix, iy, ieta).epsilon = epsilon;
            arena_current(ix, iy, ieta).rhob    = rhob;
            arena_prev   (ix, iy, ieta).rhob    = rhob;
            
            double utau_local = sqrt(1.
                          + temp_profile_ux[ix][iy]*temp_profile_ux[ix][iy]
                          + temp_profile_uy[ix][iy]*temp_profile_uy[ix][iy]);
            arena_current(ix, iy, ieta).u[0] = utau_local;
            arena_current(ix, iy, ieta).u[1] = temp_profile_ux[ix][iy];
            arena_current(ix, iy, ieta).u[2] = temp_profile_uy[ix][iy];
            arena_current(ix, iy, ieta).u[3] = 0.0;
            arena_prev(ix, iy, ieta).u = arena_current(ix, iy, ieta).u;

            if (DATA.turn_on_shear == 0) {
                double utau_prev = sqrt(1.
                    + temp_profile_ux_prev[ix][iy]*temp_profile_ux_prev[ix][iy]
                    + temp_profile_uy_prev[ix][iy]*temp_profile_uy_prev[ix][iy]
                );
                arena_prev(ix, iy, ieta).u[0] = utau_prev;
                arena_prev(ix, iy, ieta).u[1] = temp_profile_ux_prev[ix][iy];
                arena_prev(ix, iy, ieta).u[2] = temp_profile_uy_prev[ix][iy];
                arena_prev(ix, iy, ieta).u[3] = 0.0;
            }

            if (DATA.turn_on_shear == 1) {
                arena_current(ix,iy,ieta).Wmunu[0] = temp_profile_pi00[ix][iy];
                arena_current(ix,iy,ieta).Wmunu[1] = temp_profile_pi0x[ix][iy];
                arena_current(ix,iy,ieta).Wmunu[2] = temp_profile_pi0y[ix][iy];
                arena_current(ix,iy,ieta).Wmunu[3] = 0.0;
                arena_current(ix,iy,ieta).Wmunu[4] = temp_profile_pixx[ix][iy];
                arena_current(ix,iy,ieta).Wmunu[5] = temp_profile_pixy[ix][iy];
                arena_current(ix,iy,ieta).Wmunu[6] = 0.0;
                arena_current(ix,iy,ieta).Wmunu[7] = temp_profile_piyy[ix][iy];
                arena_current(ix,iy,ieta).Wmunu[8] = 0.0;
                arena_current(ix,iy,ieta).Wmunu[9] = temp_profile_pi33[ix][iy];
            }
            arena_prev(ix,iy,ieta).Wmunu = arena_current(ix,iy,ieta).Wmunu;
        }
    }
}

void Init::initial_1p1D_eta(SCGrid &arena_prev, SCGrid &arena_current) {
    std::string input_ed_filename;
    std::string input_rhob_filename;
    input_ed_filename = "tests/test_1+1D_with_Akihiko/e_baryon_init.dat";
    input_rhob_filename = "tests/test_1+1D_with_Akihiko/rhoB_baryon_init.dat";

    ifstream profile_ed(input_ed_filename.c_str());
    if (!profile_ed.good()) {
        music_message << "Init::InitTJb: "
                      << "Can not open the initial file: "
                      << input_ed_filename;
        music_message.flush("error");
        exit(1);
    }
    ifstream profile_rhob;
    profile_rhob.open(input_rhob_filename.c_str());
    if (!profile_rhob.good()) {
        music_message << "Init::InitTJb: "
                      << "Can not open the initial file: "
                      << input_rhob_filename;
        music_message.flush("error");
        exit(1);
    }

    const int neta = arena_current.nEta();
    double temp_profile_ed[neta];
    double temp_profile_rhob[neta];

    double dummy;
    for (int ieta = 0; ieta < neta; ieta++) {
        profile_ed >> dummy >> temp_profile_ed[ieta];
        profile_rhob >> dummy >> temp_profile_rhob[ieta];
    }
    profile_ed.close();
    profile_rhob.close();

    const int nx = arena_current.nX();
    const int ny = arena_current.nY();
    for (int ieta = 0; ieta < neta; ieta++) {
        double rhob = temp_profile_rhob[ieta];
        double epsilon = temp_profile_ed[ieta]/hbarc;   // fm^-4
        for (int ix = 0; ix < nx; ix++) {
            for (int iy = 0; iy< ny; iy++) {
                // set all values in the grid element:
                arena_current(ix, iy, ieta).epsilon = epsilon;
                arena_current(ix, iy, ieta).rhob    = rhob;
            
                arena_current(ix, iy, ieta).u[0] = 1.0;
                arena_current(ix, iy, ieta).u[1] = 0.0;
                arena_current(ix, iy, ieta).u[2] = 0.0;
                arena_current(ix, iy, ieta).u[3] = 0.0;

                arena_prev(ix, iy, ieta) = arena_current(ix, iy, ieta);
            }
        }
    }
}

void Init::initial_IPGlasma_XY(int ieta, SCGrid &arena_prev,
                               SCGrid &arena_current) {
    ifstream profile(DATA.initName.c_str());

    std::string dummy;
    // read the information line
    std::getline(profile, dummy);

    const int nx = arena_current.nX();
    const int ny = arena_current.nY();
    double temp_profile_ed[nx][ny];
    double temp_profile_utau[nx][ny];
    double temp_profile_ux[nx][ny];
    double temp_profile_uy[nx][ny];

    // read the one slice
    double density, dummy1, dummy2, dummy3;
    double ux, uy, utau;
    for (int ix = 0; ix < nx; ix++) {
        for (int iy = 0; iy < ny; iy++) {
            profile >> dummy1 >> dummy2 >> dummy3
                    >> density >> utau >> ux >> uy
                    >> dummy  >> dummy  >> dummy  >> dummy;
            temp_profile_ed[ix][iy] = density;
            temp_profile_ux[ix][iy] = ux;
            temp_profile_uy[ix][iy] = uy;
            temp_profile_utau[ix][iy] = sqrt(1. + ux*ux + uy*uy);
            if (ix == 0 && iy == 0) {
                DATA.x_size = -dummy2*2;
                DATA.y_size = -dummy3*2;
                if (omp_get_thread_num() == 0) {
                    music_message << "eta_size=" << DATA.eta_size
                                  << ", x_size=" << DATA.x_size
                                  << ", y_size=" << DATA.y_size;
                    music_message.flush("info");
                }
            }
        }
    }
    profile.close();

    double eta = (DATA.delta_eta)*ieta - (DATA.eta_size)/2.0;
    double eta_envelop_ed = eta_profile_normalisation(eta);
    int entropy_flag = DATA.initializeEntropy;
    for (int ix = 0; ix < nx; ix++) {
        for (int iy = 0; iy< ny; iy++) {
            double rhob = 0.0;
            double epsilon = 0.0;
            if (entropy_flag == 0) {
                epsilon = (temp_profile_ed[ix][iy]*eta_envelop_ed
                           *DATA.sFactor/hbarc);  // 1/fm^4
            } else {
                double local_sd = (temp_profile_ed[ix][iy]*DATA.sFactor
                                   *eta_envelop_ed);
                epsilon = eos.get_s2e(local_sd, rhob);
            }
            if (epsilon < 0.00000000001)
                epsilon = 0.00000000001;

            arena_current(ix, iy, ieta).epsilon = epsilon;
            arena_current(ix, iy, ieta).rhob = rhob;

            arena_current(ix, iy, ieta).u[0] = temp_profile_utau[ix][iy];
            arena_current(ix, iy, ieta).u[1] = temp_profile_ux[ix][iy];
            arena_current(ix, iy, ieta).u[2] = temp_profile_uy[ix][iy];
            arena_current(ix, iy, ieta).u[3] = 0.0;

            arena_prev(ix, iy, ieta) = arena_current(ix, iy, ieta);
        }
    }
}

void Init::initial_IPGlasma_XY_with_pi(int ieta, SCGrid &arena_prev,
                                       SCGrid &arena_current) {
    // Initial_profile == 9 : full T^\mu\nu
    // Initial_profile == 91: e and u^\mu
    // Initial_profile == 92: e only
    double tau0 = DATA.tau0;
    ifstream profile(DATA.initName.c_str());

    std::string dummy;
    // read the information line
    std::getline(profile, dummy);

    const int nx = arena_current.nX();
    const int ny = arena_current.nY();
    std::vector<double> temp_profile_ed(nx*ny, 0.0);
    std::vector<double> temp_profile_utau(nx*ny, 0.0);
    std::vector<double> temp_profile_ux(nx*ny, 0.0);
    std::vector<double> temp_profile_uy(nx*ny, 0.0);
    std::vector<double> temp_profile_ueta(nx*ny, 0.0);
    std::vector<double> temp_profile_pitautau(nx*ny, 0.0);
    std::vector<double> temp_profile_pitaux(nx*ny, 0.0);
    std::vector<double> temp_profile_pitauy(nx*ny, 0.0);
    std::vector<double> temp_profile_pitaueta(nx*ny, 0.0);
    std::vector<double> temp_profile_pixx(nx*ny, 0.0);
    std::vector<double> temp_profile_pixy(nx*ny, 0.0);
    std::vector<double> temp_profile_pixeta(nx*ny, 0.0);
    std::vector<double> temp_profile_piyy(nx*ny, 0.0);
    std::vector<double> temp_profile_piyeta(nx*ny, 0.0);
    std::vector<double> temp_profile_pietaeta(nx*ny, 0.0);

    // read the one slice
    double density, dummy1, dummy2, dummy3;
    double ux, uy, utau, ueta;
    double pitautau, pitaux, pitauy, pitaueta;
    double pixx, pixy, pixeta, piyy, piyeta, pietaeta;
    for (int ix = 0; ix < nx; ix++) {
        for (int iy = 0; iy < ny; iy++) {
            int idx = iy + ix*ny;
            std::getline(profile, dummy);
            std::stringstream ss(dummy);
            ss >> dummy1 >> dummy2 >> dummy3
               >> density >> utau >> ux >> uy >> ueta
               >> pitautau >> pitaux >> pitauy >> pitaueta
               >> pixx >> pixy >> pixeta >> piyy >> piyeta >> pietaeta;
            temp_profile_ed    [idx] = density;
            temp_profile_ux    [idx] = ux;
            temp_profile_uy    [idx] = uy;
            temp_profile_ueta  [idx] = ueta*tau0;
            temp_profile_utau  [idx] = sqrt(1. + ux*ux + uy*uy + ueta*ueta);
            temp_profile_pixx  [idx] = pixx*DATA.sFactor;
            temp_profile_pixy  [idx] = pixy*DATA.sFactor;
            temp_profile_pixeta[idx] = pixeta*tau0*DATA.sFactor;
            temp_profile_piyy  [idx] = piyy*DATA.sFactor;
            temp_profile_piyeta[idx] = piyeta*tau0*DATA.sFactor;

            utau = temp_profile_utau[idx];
            ueta = ueta*tau0;
            temp_profile_pietaeta[idx] = (
                (2.*(  ux*uy*temp_profile_pixy[idx]
                     + ux*ueta*temp_profile_pixeta[idx]
                     + uy*ueta*temp_profile_piyeta[idx])
                 - (utau*utau - ux*ux)*temp_profile_pixx[idx]
                 - (utau*utau - uy*uy)*temp_profile_piyy[idx])
                /(utau*utau - ueta*ueta));
            temp_profile_pitaux  [idx] = (1./utau
                *(  temp_profile_pixx[idx]*ux
                  + temp_profile_pixy[idx]*uy
                  + temp_profile_pixeta[idx]*ueta));
            temp_profile_pitauy  [idx] = (1./utau
                *(  temp_profile_pixy[idx]*ux
                  + temp_profile_piyy[idx]*uy
                  + temp_profile_piyeta[idx]*ueta));
            temp_profile_pitaueta[idx] = (1./utau
                *(  temp_profile_pixeta[idx]*ux
                  + temp_profile_piyeta[idx]*uy
                  + temp_profile_pietaeta[idx]*ueta));
            temp_profile_pitautau[idx] = (1./utau
                *(  temp_profile_pitaux[idx]*ux
                  + temp_profile_pitauy[idx]*uy
                  + temp_profile_pitaueta[idx]*ueta));
            if (ix == 0 && iy == 0) {
                DATA.x_size = -dummy2*2;
                DATA.y_size = -dummy3*2;
                if (omp_get_thread_num() == 0) {
                    music_message << "eta_size=" << DATA.eta_size
                                  << ", x_size=" << DATA.x_size
                                  << ", y_size=" << DATA.y_size;
                    music_message.flush("info");
                }
            }
        }
    }
    profile.close();

    double eta = (DATA.delta_eta)*(ieta) - (DATA.eta_size)/2.0;
    double eta_envelop_ed = eta_profile_normalisation(eta);
    int entropy_flag = DATA.initializeEntropy;
    for (int ix = 0; ix < nx; ix++) {
        for (int iy = 0; iy< ny; iy++) {
            int idx = iy + ix*ny;
            double rhob = 0.0;
            double epsilon = 0.0;
            if (entropy_flag == 0) {
                epsilon = (temp_profile_ed[idx]*eta_envelop_ed
                           *DATA.sFactor/hbarc);  // 1/fm^4
            } else {
                double local_sd = (temp_profile_ed[idx]*DATA.sFactor
                                   *eta_envelop_ed);
                epsilon = eos.get_s2e(local_sd, rhob);
            }
            if (epsilon < 0.00000000001)
                epsilon = 0.00000000001;

            arena_current(ix, iy, ieta).epsilon = epsilon;
            arena_current(ix, iy, ieta).rhob = rhob;

            if (DATA.Initial_profile == 9 || DATA.Initial_profile == 91) {
                arena_current(ix, iy, ieta).u[0] = temp_profile_utau[idx];
                arena_current(ix, iy, ieta).u[1] = temp_profile_ux[idx];
                arena_current(ix, iy, ieta).u[2] = temp_profile_uy[idx];
                arena_current(ix, iy, ieta).u[3] = temp_profile_ueta[idx];
            } else {
                arena_current(ix, iy, ieta).u[0] = 1.0;
                arena_current(ix, iy, ieta).u[1] = 0.0;
                arena_current(ix, iy, ieta).u[2] = 0.0;
                arena_current(ix, iy, ieta).u[3] = 0.0;
            }
            
            if (DATA.Initial_profile == 9) {
                double pressure = eos.get_pressure(epsilon, rhob);
                arena_current(ix, iy, ieta).pi_b = epsilon/3. - pressure;

                arena_current(ix, iy, ieta).Wmunu[0] = temp_profile_pitautau[idx];
                arena_current(ix, iy, ieta).Wmunu[1] = temp_profile_pitaux[idx];
                arena_current(ix, iy, ieta).Wmunu[2] = temp_profile_pitauy[idx];
                arena_current(ix, iy, ieta).Wmunu[3] = temp_profile_pitaueta[idx];
                arena_current(ix, iy, ieta).Wmunu[4] = temp_profile_pixx[idx];
                arena_current(ix, iy, ieta).Wmunu[5] = temp_profile_pixy[idx];
                arena_current(ix, iy, ieta).Wmunu[6] = temp_profile_pixeta[idx];
                arena_current(ix, iy, ieta).Wmunu[7] = temp_profile_piyy[idx];
                arena_current(ix, iy, ieta).Wmunu[8] = temp_profile_piyeta[idx];
                arena_current(ix, iy, ieta).Wmunu[9] = temp_profile_pietaeta[idx];
            }

            arena_prev(ix, iy, ieta) = arena_current(ix, iy, ieta);
        }
    }
}

void Init::initial_MCGlb_with_rhob_XY(int ieta, SCGrid &arena_prev,
                                      SCGrid &arena_current) {
    // first load in the transverse profile
    ifstream profile_TA(DATA.initName_TA.c_str());
    ifstream profile_TB(DATA.initName_TB.c_str());
    ifstream profile_rhob_TA(DATA.initName_rhob_TA.c_str());
    ifstream profile_rhob_TB(DATA.initName_rhob_TB.c_str());

    const int nx = arena_current.nX();
    const int ny = arena_current.nY();
    double temp_profile_TA[nx][ny];
    double temp_profile_TB[nx][ny];
    double temp_profile_rhob_TA[nx][ny];
    double temp_profile_rhob_TB[nx][ny];
    for (int i = 0; i < nx; i++) {
        for (int j = 0; j < ny; j++) {
            profile_TA >> temp_profile_TA[i][j];
            profile_TB >> temp_profile_TB[i][j];
            profile_rhob_TA >> temp_profile_rhob_TA[i][j];
            profile_rhob_TB >> temp_profile_rhob_TB[i][j];
        }
    }
    profile_TA.close();
    profile_TB.close();
    profile_rhob_TA.close();
    profile_rhob_TB.close();

    double eta = (DATA.delta_eta)*ieta - (DATA.eta_size)/2.0;
    double eta_envelop_left  = eta_profile_left_factor(eta);
    double eta_envelop_right = eta_profile_right_factor(eta);
    double eta_rhob_left     = eta_rhob_left_factor(eta);
    double eta_rhob_right    = eta_rhob_right_factor(eta);

    int entropy_flag = DATA.initializeEntropy;
    for (int ix = 0; ix < nx; ix++) {
        for (int iy = 0; iy< ny; iy++) {
            double rhob = 0.0;
            double epsilon = 0.0;
            if (DATA.turn_on_rhob == 1) {
                rhob = (
                    (temp_profile_rhob_TA[ix][iy]*eta_rhob_left
                     + temp_profile_rhob_TB[ix][iy]*eta_rhob_right));
            } else {
                rhob = 0.0;
            }
            if (entropy_flag == 0) {
                epsilon = (
                    (temp_profile_TA[ix][iy]*eta_envelop_left
                     + temp_profile_TB[ix][iy]*eta_envelop_right)
                    *DATA.sFactor/hbarc);   // 1/fm^4
            } else {
                double local_sd = (
                    (temp_profile_TA[ix][iy]*eta_envelop_left
                     + temp_profile_TB[ix][iy]*eta_envelop_right)
                    *DATA.sFactor);         // 1/fm^3
                epsilon = eos.get_s2e(local_sd, rhob);
            }
            if (epsilon < 0.00000000001)
                epsilon = 0.00000000001;

            arena_current(ix, iy, ieta).epsilon = epsilon;
            arena_current(ix, iy, ieta).rhob = rhob;

            arena_current(ix, iy, ieta).u[0] = 1.0;
            arena_current(ix, iy, ieta).u[1] = 0.0;
            arena_current(ix, iy, ieta).u[2] = 0.0;
            arena_current(ix, iy, ieta).u[3] = 0.0;

            arena_prev(ix, iy, ieta) = arena_current(ix, iy, ieta);
        }
    }
}

void Init::initial_MCGlbLEXUS_with_rhob_XY(int ieta, SCGrid &arena_prev,
                                           SCGrid &arena_current) {
    const int nx = arena_current.nX();
    const int ny = arena_current.nY();
    double u[4] = {1.0, 0.0, 0.0, 0.0};
    for (int ix = 0; ix < nx; ix++) {
        for (int iy = 0; iy < ny; iy++) {
            double rhob = 0.0;
            double epsilon = 1e-12;

            arena_current(ix, iy, ieta).epsilon = epsilon;
            arena_current(ix, iy, ieta).rhob = rhob;

            arena_current(ix, iy, ieta).u[0] = u[0];
            arena_current(ix, iy, ieta).u[1] = u[1];
            arena_current(ix, iy, ieta).u[2] = u[2];
            arena_current(ix, iy, ieta).u[3] = u[3];
            
            arena_prev(ix, iy, ieta) = arena_current(ix, iy, ieta);
        }
    }
}


void Init::initial_UMN_with_rhob(SCGrid &arena_prev, SCGrid &arena_current) {
    // first load in the transverse profile
    ifstream profile(DATA.initName.c_str());
    ifstream profile_TA(DATA.initName_TA.c_str());
    ifstream profile_TB(DATA.initName_TB.c_str());

    const int nx = arena_current.nX();
    const int ny = arena_current.nY();
    double temp_profile_TA[nx][ny];
    double temp_profile_TB[nx][ny];
    for (int i = 0; i < nx; i++) {
        for (int j = 0; j < ny; j++) {
            profile_TA >> temp_profile_TA[i][j];
            profile_TB >> temp_profile_TB[i][j];
        }
    }
    profile_TA.close();
    profile_TB.close();

    double dummy;
    double ed_local, rhob_local;
    for (int ieta = 0; ieta < DATA.neta; ieta++) {
        double eta = (DATA.delta_eta)*ieta - (DATA.eta_size)/2.0;
        double eta_envelop_left = eta_profile_left_factor(eta);
        double eta_envelop_right = eta_profile_right_factor(eta);
        for (int ix = 0; ix < nx; ix++) {
            for (int iy = 0; iy< ny; iy++) {
                double rhob = 0.0;
                double epsilon = 0.0;
                profile >> dummy >> dummy >> dummy >> ed_local >> rhob_local;
                rhob = rhob_local;
                double sd_bg = (
                      temp_profile_TA[ix][iy]*eta_envelop_left
                    + temp_profile_TB[ix][iy]*eta_envelop_right)*DATA.sFactor;
                double ed_bg = eos.get_s2e(sd_bg, 0.0);
                epsilon = ed_bg + ed_local/hbarc;    // 1/fm^4
                if (epsilon < 0.00000000001) {
                    epsilon = 0.00000000001;
                }

                arena_current(ix, iy, ieta).epsilon = epsilon;
                arena_current(ix, iy, ieta).rhob = rhob;

                arena_current(ix, iy, ieta).u[0] = 1.0;
                arena_current(ix, iy, ieta).u[1] = 0.0;
                arena_current(ix, iy, ieta).u[2] = 0.0;
                arena_current(ix, iy, ieta).u[3] = 0.0;
            
                arena_prev(ix, iy, ieta) = arena_current(ix, iy, ieta);
            }
        }
    }
    profile.close();
}

void Init::initial_AMPT_XY(int ieta, SCGrid &arena_prev,
                           SCGrid &arena_current) {
    double u[4] = {1.0, 0.0, 0.0, 0.0};
    double j_mu[4] = {0.0, 0.0, 0.0, 0.0};

    double eta = (DATA.delta_eta)*ieta - (DATA.eta_size)/2.0;
    double tau0 = DATA.tau0;
    const int nx = arena_current.nX();
    const int ny = arena_current.nY();
    for (int ix = 0; ix < nx; ix++) {
        double x_local = - DATA.x_size/2. + ix*DATA.delta_x;
        for (int iy = 0; iy < ny; iy++) {
            double y_local = - DATA.y_size/2. + iy*DATA.delta_y;
            double rhob = 0.0;
            double epsilon = 0.0;
            if (DATA.turn_on_rhob == 1) {
                rhob = hydro_source_terms.get_hydro_rhob_source_before_tau(
                                                tau0, x_local, y_local, eta);
            } else {
                rhob = 0.0;
            }

            hydro_source_terms.get_hydro_energy_source_before_tau(
                                    tau0, x_local, y_local, eta, j_mu);

            epsilon = j_mu[0];           // 1/fm^4

            if (epsilon < 0.00000000001)
                epsilon = 0.00000000001;

            arena_current(ix, iy, ieta).epsilon = epsilon;
            arena_current(ix, iy, ieta).rhob = rhob;

            arena_current(ix, iy, ieta).u[0] = u[0];
            arena_current(ix, iy, ieta).u[1] = u[1];
            arena_current(ix, iy, ieta).u[2] = u[2];
            arena_current(ix, iy, ieta).u[3] = u[3];
            
            arena_prev(ix, iy, ieta) = arena_current(ix, iy, ieta);
        }
    }
}


void Init::get_jetscape_preequilibrium_vectors(
        vector<double> e_in,
        vector<double> u_tau_in, vector<double> u_x_in,
        vector<double> u_y_in,   vector<double> u_eta_in,
        vector<double> pi_00_in, vector<double> pi_01_in,
        vector<double> pi_02_in, vector<double> pi_03_in,
        vector<double> pi_11_in, vector<double> pi_12_in,
        vector<double> pi_13_in, vector<double> pi_22_in,
        vector<double> pi_23_in, vector<double> pi_33_in,
        vector<double> Bulk_pi_in) {
    jetscape_initial_energy_density = e_in;
    jetscape_initial_u_tau          = u_tau_in;
    jetscape_initial_u_x            = u_x_in;
    jetscape_initial_u_y            = u_y_in;
    jetscape_initial_u_eta          = u_eta_in;
    jetscape_initial_pi_00          = pi_00_in;
    jetscape_initial_pi_01          = pi_01_in;
    jetscape_initial_pi_02          = pi_02_in;
    jetscape_initial_pi_03          = pi_03_in;
    jetscape_initial_pi_11          = pi_11_in;
    jetscape_initial_pi_12          = pi_12_in;
    jetscape_initial_pi_13          = pi_13_in;
    jetscape_initial_pi_22          = pi_22_in;
    jetscape_initial_pi_23          = pi_23_in;
    jetscape_initial_pi_33          = pi_33_in;
    jetscape_initial_bulk_pi        = Bulk_pi_in;
}


void Init::initial_with_jetscape(int ieta, SCGrid &arena_prev,
                                 SCGrid &arena_current) {
    const int nx = arena_current.nX();
    const int ny = arena_current.nY();
    
    for (int ix = 0; ix < nx; ix++) {
        for (int iy = 0; iy< ny; iy++) {
            const double rhob = 0.0;
            double epsilon = 0.0;
            const int idx = ix + (iy + ieta*nx)*nx;
            epsilon = (jetscape_initial_energy_density[idx]
                       *DATA.sFactor/hbarc);  // 1/fm^4
            if (epsilon < 0.00000000001)
                epsilon = 0.00000000001;

            arena_current(ix, iy, ieta).epsilon = epsilon;
            arena_current(ix, iy, ieta).rhob = rhob;

            arena_current(ix, iy, ieta).u[0] = jetscape_initial_u_tau[idx];
            arena_current(ix, iy, ieta).u[1] = jetscape_initial_u_x[idx];
            arena_current(ix, iy, ieta).u[2] = jetscape_initial_u_y[idx];
            arena_current(ix, iy, ieta).u[3] = DATA.tau0*jetscape_initial_u_eta[idx];

            arena_current(ix, iy, ieta).pi_b = jetscape_initial_bulk_pi[idx]/hbarc;

            arena_current(ix, iy, ieta).Wmunu[0] = jetscape_initial_pi_00[idx]/hbarc;
            arena_current(ix, iy, ieta).Wmunu[1] = jetscape_initial_pi_01[idx]/hbarc;
            arena_current(ix, iy, ieta).Wmunu[2] = jetscape_initial_pi_02[idx]/hbarc;
            arena_current(ix, iy, ieta).Wmunu[3] = jetscape_initial_pi_03[idx]/hbarc*DATA.tau0;
            arena_current(ix, iy, ieta).Wmunu[4] = jetscape_initial_pi_11[idx]/hbarc;
            arena_current(ix, iy, ieta).Wmunu[5] = jetscape_initial_pi_12[idx]/hbarc;
            arena_current(ix, iy, ieta).Wmunu[6] = jetscape_initial_pi_13[idx]/hbarc*DATA.tau0;
            arena_current(ix, iy, ieta).Wmunu[7] = jetscape_initial_pi_22[idx]/hbarc;
            arena_current(ix, iy, ieta).Wmunu[8] = jetscape_initial_pi_23[idx]/hbarc*DATA.tau0;
            arena_current(ix, iy, ieta).Wmunu[9] = jetscape_initial_pi_33[idx]/hbarc*DATA.tau0;

            arena_prev(ix, iy, ieta) = arena_current(ix, iy, ieta);
        }
    }
}

void Init::clean_up_jetscape_arrays() {
    // clean up
    jetscape_initial_energy_density.clear();
    jetscape_initial_u_tau.clear();
    jetscape_initial_u_x.clear();
    jetscape_initial_u_y.clear();
    jetscape_initial_u_eta.clear();
    jetscape_initial_pi_00.clear();
    jetscape_initial_pi_01.clear();
    jetscape_initial_pi_02.clear();
    jetscape_initial_pi_03.clear();
    jetscape_initial_pi_11.clear();
    jetscape_initial_pi_12.clear();
    jetscape_initial_pi_13.clear();
    jetscape_initial_pi_22.clear();
    jetscape_initial_pi_23.clear();
    jetscape_initial_pi_33.clear();
    jetscape_initial_bulk_pi.clear();
}

double Init::eta_profile_normalisation(double eta) {
    // this function return the eta envelope profile for energy density
    double res;
    // Hirano's plateau + Gaussian fall-off
    if (DATA.initial_eta_profile == 1) {
        double exparg1 = (fabs(eta) - DATA.eta_flat/2.0)/DATA.eta_fall_off;
        double exparg = exparg1*exparg1/2.0;
        res = exp(-exparg*theta(exparg1));
    } else if (DATA.initial_eta_profile == 2) {
        // Woods-Saxon
        // The radius is set to be half of DATA.eta_flat
        // The diffusiveness is set to DATA.eta_fall_off
        double ws_R = DATA.eta_flat/2.0;
        double ws_a = DATA.eta_fall_off;
        res = (1.0 + exp(-ws_R/ws_a))/(1.0 + exp((abs(eta) - ws_R)/ws_a));
    } else {
        music_message.error("initial_eta_profile out of range.");
        exit(1);
    }
    return res;
}

double Init::eta_profile_left_factor(double eta) {
    // this function return the eta envelope for projectile
    double res = eta_profile_normalisation(eta);
    if (fabs(eta) < DATA.beam_rapidity) {
        res = (1. - eta/DATA.beam_rapidity)*res;
    } else {
        res = 0.0;
    }
    return(res);
}

double Init::eta_profile_right_factor(double eta) {
    // this function return the eta envelope for target
    double res = eta_profile_normalisation(eta);
    if (fabs(eta) < DATA.beam_rapidity) {
        res = (1. + eta/DATA.beam_rapidity)*res;
    } else {
        res = 0.0;
    }
    return(res);
}

double Init::eta_rhob_profile_normalisation(double eta) {
    // this function return the eta envelope profile for net baryon density
    double res;
    int profile_flag = DATA.initial_eta_rhob_profile;
    double eta_0 = DATA.eta_rhob_0;
    double tau0 = DATA.tau0;
    if (profile_flag == 1) {
        const double eta_width = DATA.eta_rhob_width;
        const double norm      = 1./(2.*sqrt(2*M_PI)*eta_width*tau0);
        const double exparg1   = (eta - eta_0)/eta_width;
        const double exparg2   = (eta + eta_0)/eta_width;
        res = norm*(exp(-exparg1*exparg1/2.0) + exp(-exparg2*exparg2/2.0));
    } else if (profile_flag == 2) {
        double eta_abs     = fabs(eta);
        double delta_eta_1 = DATA.eta_rhob_width_1;
        double delta_eta_2 = DATA.eta_rhob_width_2;
        double A           = DATA.eta_rhob_plateau_height;
        double exparg1     = (eta_abs - eta_0)/delta_eta_1;
        double exparg2     = (eta_abs - eta_0)/delta_eta_2;
        double theta;
        double norm = 1./(tau0*(sqrt(2.*M_PI)*delta_eta_1
                          + (1. - A)*sqrt(2.*M_PI)*delta_eta_2 + 2.*A*eta_0));
        if (eta_abs > eta_0)
            theta = 1.0;
        else
            theta = 0.0;
        res = norm*(theta*exp(-exparg1*exparg1/2.)
                    + (1. - theta)*(A + (1. - A)*exp(-exparg2*exparg2/2.)));
    } else {
        music_message << "initial_eta_rhob_profile = " << profile_flag
                      << " out of range.";
        music_message.flush("error");
        exit(1);
    }
    return res;
}

double Init::eta_rhob_left_factor(double eta) {
    double eta_0       = -fabs(DATA.eta_rhob_0);
    double tau0        = DATA.tau0;
    double delta_eta_1 = DATA.eta_rhob_width_1;
    double delta_eta_2 = DATA.eta_rhob_width_2;
    double norm        = 2./(sqrt(M_PI)*tau0*(delta_eta_1 + delta_eta_2));
    double exp_arg     = 0.0;
    if (eta < eta_0) {
        exp_arg = (eta - eta_0)/delta_eta_1;
    } else {
        exp_arg = (eta - eta_0)/delta_eta_2;
    }
    double res = norm*exp(-exp_arg*exp_arg);
    return(res);
}

double Init::eta_rhob_right_factor(double eta) {
    double eta_0       = fabs(DATA.eta_rhob_0);
    double tau0        = DATA.tau0;
    double delta_eta_1 = DATA.eta_rhob_width_1;
    double delta_eta_2 = DATA.eta_rhob_width_2;
    double norm        = 2./(sqrt(M_PI)*tau0*(delta_eta_1 + delta_eta_2));
    double exp_arg     = 0.0;
    if (eta < eta_0) {
        exp_arg = (eta - eta_0)/delta_eta_2;
    } else {
        exp_arg = (eta - eta_0)/delta_eta_1;
    }
    double res = norm*exp(-exp_arg*exp_arg);
    return(res);
}

void Init::output_initial_density_profiles(SCGrid &arena) {
    // this function outputs the 3d initial energy density profile
    // and net baryon density profile (if turn_on_rhob == 1)
    // for checking purpose
    music_message.info("output initial density profiles into a file... ");
    std::ofstream of("check_initial_density_profiles.dat");
    of << "# x(fm)  y(fm)  eta  ed(GeV/fm^3)";
    if (DATA.turn_on_rhob == 1)
        of << "  rhob(1/fm^3)";
    of << std::endl;
    for (int ieta = 0; ieta < arena.nEta(); ieta++) {
        double eta_local = (DATA.delta_eta)*ieta - (DATA.eta_size)/2.0;
        for(int ix = 0; ix < arena.nX(); ix++) {
            double x_local = -DATA.x_size/2. + ix*DATA.delta_x;
            for(int iy = 0; iy < arena.nY(); iy++) {
                double y_local = -DATA.y_size/2. + iy*DATA.delta_y;
                of << std::scientific << std::setw(18) << std::setprecision(8)
                   << x_local << "   " << y_local << "   "
                   << eta_local << "   " << arena(ix,iy,ieta).epsilon*hbarc;
                if (DATA.turn_on_rhob == 1) {
                    of << "   " << arena(ix,iy,ieta).rhob;
                }
                of << std::endl;
            }
        }
    }
    music_message.info("done!");
}
