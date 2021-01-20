/*
 * An example on how to use pset in libmpidynres and how to get more information
 * about the resources contained in a pset
 */
#include <mpi.h>
#include <mpidynres.h>
#include <mpidynres_pset.h>
#include <mpidynres_sim.h>
#include <stdio.h>
#include <stdlib.h>

int print_pset_info(MPI_Session mysession) {
  int err, num_psets;
  MPI_Info info;

  printf("Hallo\n");

  err = MPI_Session_get_num_psets(mysession, MPI_INFO_NULL, &num_psets);
  if (err) {
    printf("Something went wrong\n");
    return err;
  }
  printf("Number of psets containing me: %d\n", num_psets);

  for (int i = 0; i < num_psets; i++) {
    char pset_name[MPI_MAX_PSET_NAME_LEN];
    int name_len = MPI_MAX_PSET_NAME_LEN;

    err = MPI_Session_get_nth_pset(mysession, MPI_INFO_NULL, i, &name_len,
                                   pset_name);
    if (err) {
      printf("Something went wrong\n");
      return err;
    }

    printf("pset number %d is called %s\n", i, pset_name);

    err = MPI_Session_get_pset_info(mysession, pset_name, &info);
    if (err) {
      printf("Something went wrong\n");
      return err;
    }
  }

  return 0;
}

int MPIDYNRES_main(int argc, char *argv[]) {
  (void)argc, (void)argv;
  MPI_Session mysession;
  MPI_Info info;
  int err;

  err = MPI_Session_init(MPI_INFO_NULL, MPI_ERRORS_ARE_FATAL, &mysession);
  if (err) {
    printf("Something went wrong\n");
    return err;
  }

  err = MPI_Session_get_info(mysession, &info);
  if (err) {
    printf("Something went wrong\n");
    return err;
  }

  print_pset_info(mysession);

  err = MPI_Session_finalize(&mysession);
  if (err) {
    printf("Something went wrong\n");
    return err;
  }

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
