/*
 * An example on
 */
#include <mpidynres.h>
#include <mpidynres_sim.h>
#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>

int MPIDYNRES_main(int argc, char *argv[]) {
  (void)argc, (void)argv;
  MPI_Session mysession;
  MPI_Info info;
  int err;

  printf("Is everything just a simulation?\n");


  err = MPI_Session_init(MPI_INFO_NULL, MPI_ERRORS_ARE_FATAL, &mysession);
  if (err) {
    printf("Something went wrong\n");
    return err;
  }

  printf("%d\n", mysession->session_id);
  err = MPI_Session_get_info(mysession, &info);
  if (err) {
    printf("Something went wrong\n");
    return err;
  }

  err = MPI_Session_finalize(&mysession);
  if (err) {
    printf("Something went wrong\n");
    return err;
  }

  printf("%d\n", mysession == MPI_SESSION_INVALID);

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