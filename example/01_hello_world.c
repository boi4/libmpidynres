/*
 * A simple hello world example using libmpidynres.
 * The program uses MPIDYNRESSIM_start_sim to start the simulation
 */
#include <mpidynres.h>
#include <mpidynres_sim.h>
#include <mpi.h>
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

  // either use mpidynres_exit to exit the simulation or just return (this
  // statement therefore is optional)
  MPIDYNRES_exit();
  return 0;
}

int main(int argc, char *argv[static argc + 1]) {
  // Please take a look at mpidynres_sim.h to see all options you can set
  MPIDYNRESSIM_config my_running_config = {
      .base_communicator = MPI_COMM_WORLD,  // simulate on MPI_COMM_WORLD
      .num_init_crs = 1,                    // start with one process
      .scheduling_mode = MPIDYNRES_MODE_INC,               // add new processes incrementally
      .change_prob = 0.1,                   // the resource change probability
  };

  // Don't forget to init and finalize mpi yourself!
  MPI_Init(&argc, &argv);

  // This call will start the simulation immediatly (and block until the
  // simualtion has finished)
  MPIDYNRESSIM_start_sim(my_running_config, argc, argv, MPIDYNRES_main);

  MPI_Finalize();

  return EXIT_SUCCESS;
}