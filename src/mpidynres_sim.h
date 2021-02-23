/*
 * This file should be included when you want to start a simulated dynamic mpi
 * session
 */
#ifndef MPIDYNRESSIM_H
#define MPIDYNRESSIM_H

#include <mpi.h>

#include "mpidynres.h"

/**
 * @brief A configuration object describes how the scheduler behaves and should
 * be passed together with MPIDYNRES_start_sim
 */
enum scheduling_mode {
  MPIDYNRES_MODE_RANDOM_DIFF,
  MPIDYNRES_MODE_INC,
  MPIDYNRES_MODE_INC_DEC
};

/**
 * @brief      this struct can be used to change the behaviour of mpidynres and its
 * scheduling
 */
struct MPIDYNRESSIM_config {
  /*
   * The base_communicator contains all computing resources available for the
   * simulation
   */
  MPI_Comm base_communicator;
  MPI_Info manager_config;
};
typedef struct MPIDYNRESSIM_config MPIDYNRESSIM_config;

/*
 * MPIDYNRESSIM_get_default_config returns the a default config struct
 */
int MPIDYNRESSIM_get_default_config(MPIDYNRESSIM_config *o_config);

/*
 * MPIDYNRESSIM_Start_sim will start the simulation and will call i_sim_main
 * whenever the current computing resource is started (simulation-wise) Note
 * that MPI_Init has to be called before this function
 */
int MPIDYNRESSIM_start_sim(MPIDYNRESSIM_config i_config, int argc, char *argv[],
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
  MPIDYNRESSIM_config c;

  srand(time(NULL));
  err = MPI_Init(&argc, &argv);
  if (err) {
    fprintf(stderr, "Failed to initialize MPI\n");
    MPI_Finalize();
    exit(EXIT_FAILURE);
  }
  MPIDYNRESSIM_get_default_config(&c);
  int r = MPIDYNRESSIM_start_sim(c, argc, argv, MPIDYNRES_main);
  MPI_Finalize();
  return r;
}
#endif  // MPIDYNRES_MAIN

#endif  // header guard
