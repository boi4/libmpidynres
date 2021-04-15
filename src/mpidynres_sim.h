/*
 * This file should be included when you want to start a simulated dynamic mpi
 * session
 */
#ifndef MPIDYNRES_SIM_H
#define MPIDYNRES_SIM_H

#include <mpi.h>

#include "mpidynres.h"

/**
 * @brief      this struct can be used to change the behaviour of mpidynres and its
 * scheduling
 */
struct MPIDYNRES_SIM_config {
  /*
   * The base_communicator contains all computing resources available for the
   * simulation
   */
  MPI_Comm base_communicator;
  MPI_Info manager_config;
};
typedef struct MPIDYNRES_SIM_config MPIDYNRES_SIM_config;

/*
 * MPIDYNRES_SIM_get_default_config returns the a default config struct
 */
int MPIDYNRES_SIM_get_default_config(MPIDYNRES_SIM_config *o_config);

/*
 * MPIDYNRES_SIM_start will start the simulation and will call i_sim_main
 * whenever the current computing resource is started (simulation-wise) Note
 * that MPI_Init has to be called before this function
 */
int MPIDYNRES_SIM_start(MPIDYNRES_SIM_config i_config, int argc, char *argv[],
                        int i_sim_main(int, char **));

/*
 * You can make your program to automatically run in simulated mode
 * For that, create a MPIDYNRES_main function instead of the usual main function
 * Before importing this header, define MPIDYNRES_MAIN
 * The MPI_Sessions simulation will then use MPI_COMM_WORLD as its Computing
 * Resource Pool
 */
#ifdef MPIDYNRES_MAIN
#ifndef MPIDYNRES_NUM_INIT_RANKS
#define MPIDYNRES_NUM_INIT_RANKS 1
#endif
extern int MPIDYNRES_main(int argc, char **argv);
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
int main(int argc, char *argv[]) {
  int err;
  MPIDYNRES_SIM_config c;

  srand(time(NULL));
  err = MPI_Init(&argc, &argv);
  if (err) {
    fprintf(stderr, "Failed to initialize MPI\n");
    MPI_Finalize();
    exit(EXIT_FAILURE);
  }
  MPIDYNRES_SIM_get_default_config(&c);
  int r = MPIDYNRES_SIM_start(c, argc, argv, MPIDYNRES_main);
  MPI_Finalize();
  return r;
}
#endif  // MPIDYNRES_MAIN

#endif  // header guard
