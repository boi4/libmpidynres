/*
 * This example shows how to create MPI Communicators based on an URI
 */
#include <mpidynres.h>
#include <mpidynres_pset.h>
#include <mpidynres_sim.h>
#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define HELLO_STR "HELLO FELLOW COMPUTING RESOURCES!"

int MPIDYNRES_main(int argc, char *argv[]) {
  MPIDYNRES_init_info init_info = {0};
  MPI_Comm new_comm;
  int myrank, size;
  char buf[sizeof(HELLO_STR)];

  (void)argc, (void)argv;

  MPIDYNRES_init_info_get(&init_info);

  /*
   * Use MPIDYNRES_Comm_create_uri to create a new communicator from all crs inside
   * an uri Note that this function blocks until all other computing resources
   * contained in the uri have called this function
   * be careful with deadlocks!
   */
  MPIDYNRES_Comm_create_uri(init_info.uri_init, &new_comm);

  /*
   * Now, you can use normal MPI functions for communication
   */
  MPI_Barrier(new_comm);

  MPI_Comm_rank(new_comm, &myrank);
  MPI_Comm_size(new_comm, &size);

  printf("My rank is %d and the uri %s contains %d computing resources\n",
         myrank, init_info.uri_init, size);

  if (myrank == 0) {
    strcpy(buf, HELLO_STR);
  }

  MPI_Bcast(buf, sizeof(HELLO_STR), MPI_CHAR, 0, new_comm);

  switch (myrank) {
    case 0:
      printf("I, rank %d, sent the following string: %s\n", myrank, buf);
      break;
    default:
      printf("I, rank %d, received the following string: %s\n", myrank, buf);
      break;
  }

  /*
   * Free the communicator, after you are done!
   */
  MPI_Comm_free(&new_comm);

  return EXIT_SUCCESS;
}

int main(int argc, char *argv[static argc + 1]) {
  int world_size;

  MPI_Init(&argc, &argv);
  MPI_Comm_size(MPI_COMM_WORLD, &world_size);

  MPIDYNRESSIM_config my_running_config = {
      .base_communicator = MPI_COMM_WORLD,
      .num_init_crs = world_size - 1,  // we will start with all crs
      .scheduling_mode = MPIDYNRES_MODE_INC,
  };

  MPIDYNRESSIM_start_sim(my_running_config, argc, argv, MPIDYNRES_main);

  MPI_Finalize();
}
