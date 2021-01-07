/*
 * An example on how to get information inside the simulation about the current
 * environment
 * MPIDYNRES_init_info_get is used to fill a struct with relevant
 * information. This information is then printed
 */
#include <mpidynres.h>
#include <mpidynres_sim.h>
#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>

int MPIDYNRES_main(int argc, char *argv[]) {
  /*
   * The MPIDYNRES_init_info struct holds information about the current process and
   * its invocation
   */
  MPIDYNRES_init_info init_info = {0};

  (void)argc, (void)argv;

  printf("Is everything just a simulation?\n");

  /*
   * You can use MPIDYNRES_init_info_get to fill an init_info struct with
   * information
   */
  MPIDYNRES_init_info_get(&init_info);
  printf(
      "My init_info: cr_id: %d, init_uri: %s, passed_uri: %s, init_tag: %d\n",
      init_info.cr_id, init_info.uri_init, init_info.uri_passed,
      init_info.init_tag);

  return EXIT_SUCCESS;
}

int main(int argc, char *argv[static argc + 1]) {
  MPIDYNRESSIM_config my_running_config = {
      .base_communicator = MPI_COMM_WORLD,
      .num_init_crs = 1,
      .scheduling_mode = MPIDYNRES_MODE_INC,
      .change_prob = 0.1,
  };

  MPI_Init(&argc, &argv);
  MPIDYNRESSIM_start_sim(my_running_config, argc, argv, MPIDYNRES_main);
  MPI_Finalize();
}
