/*
 * An example on how to use URIs in libmpidynres and how to get more information
 * about the resources contained in an URI
 */
#include <mpidynres.h>
#include <mpidynres_pset.h>
#include <mpidynres_sim.h>
#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>

void print_URI_info(char *uri) {
  size_t size;
  MPIDYNRES_pset *set;

  /*
   * Use MPIDYNRES_URI_size to get the number of crs contained in a uri
   */
  MPIDYNRES_URI_size(uri, &size);
  printf("The URI %s contains %zu computing resources:\n", uri, size);

  /*
   * Use MPIDYNRES_URI_lookup to get a cr set, a datastructure, of all cr sets
   */
  MPIDYNRES_URI_lookup(uri, &set);

  /*
   * Iterate over the set like this. The cr_ids are saved sorted
   */
  for (size_t i = 0; i < set->size; i++) {
    printf("\tCR ID: %2d\n", set->cr_ids[i]);
  }

  MPIDYNRES_pset_destroy(set);
}

int MPIDYNRES_main(int argc, char *argv[]) {
  MPIDYNRES_init_info init_info = {0};

  (void)argc, (void)argv;

  MPIDYNRES_init_info_get(&init_info);

  print_URI_info(init_info.uri_init);

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
