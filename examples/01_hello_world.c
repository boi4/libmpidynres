/*
 * A simple hello world example using libmpidynres.
 * The program uses MPIDYNRES_SIM_start to start the simulation
 */
#include <mpi.h>
#include <mpidynres.h>
#include <mpidynres_sim.h>
#include <stdio.h>
#include <stdlib.h>

/*
  This function will be used as an entry point for all simulation processes

  In this case it will simply print to the terminal and exit (the exit status is
  ignored for now)
 */
int MPIDYNRES_main(int argc, char *argv[]) {
  (void)argc, (void)argv;
  printf("Is everything just a simulation?\n");

  // either use mpidynres_exit to exit the simulation or just return
  MPIDYNRES_exit();
  return 0;
}

int main(int argc, char *argv[static argc + 1]) {
  MPIDYNRES_SIM_config my_running_config = {
      .base_communicator = MPI_COMM_WORLD,  // simulate on MPI_COMM_WORLD
      .manager_config = MPI_INFO_NULL,
  };

  // Don't forget to init and finalize mpi yourself!
  MPI_Init(&argc, &argv);

  // This call will start the simulation immediatly (and block until the
  // simualtion has finished)
  MPIDYNRES_SIM_start(my_running_config, argc, argv, MPIDYNRES_main);

  MPI_Finalize();

  return EXIT_SUCCESS;
}
