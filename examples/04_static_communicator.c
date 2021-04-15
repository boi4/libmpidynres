/*
 * This example shows how to create MPI Communicators based on a pset
 */
#include <mpi.h>
#include <mpidynres.h>
#include <mpidynres_sim.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define HELLO_STR "HELLO FELLOW COMPUTING RESOURCES!"

// helper to print an mpi info object
void print_mpi_info(MPI_Info info) {
  char key[MPI_MAX_INFO_KEY + 1] = {0};
  int nkeys, vlen;
  int unused = 0;
  MPI_Info_get_nkeys(info, &nkeys);
  printf("\nMPI INFO\n");
  printf("===================\n");
  printf("{\n");
  for (int i = 0; i < nkeys; i++) {
    MPI_Info_get_nthkey(info, i, key);
    MPI_Info_get_valuelen(info, key, &vlen, &unused);
    char *val = calloc(1, vlen + 1);
    if (!val) {
      printf("Memory Error!\n");
      exit(1);
    }
    MPI_Info_get(info, key, vlen, val, &unused);
    printf("  %s: %s,\n", key, val);
    free(val);
  }
  printf("}\n");
  printf("\n\n\n");
}

int MPIDYNRES_main(int argc, char *argv[]) {
  MPI_Session mysession;
  MPI_Group mygroup;
  MPI_Comm mycomm;
  MPI_Info info;
  int myrank, size;
  char buf[sizeof(HELLO_STR)];
  int err;

  (void)argc, (void)argv;

  err = MPI_Session_init(MPI_INFO_NULL, MPI_ERRORS_ARE_FATAL, &mysession);
  if (err) {
    printf("Something went wrong\n");
    return err;
  }

  printf(
      "Session Info\n"
      "===================\n");
  err = MPI_Session_get_info(mysession, &info);
  if (err) {
    printf("Something went wrong\n");
    return err;
  }
  print_mpi_info(info);
  MPI_Info_free(&info);

  /*
   * Use MPI_Group_from_session_pset to create a new mpi group with all
   * processes in a process set All processes at the beginning will be in a
   * process set "mpi://WORLD"
   */
  err = MPI_Group_from_session_pset(mysession, "mpi://WORLD", &mygroup);
  if (err) {
    printf("Something went wrong\n");
    return err;
  }

  /*
   * Use MPI_Comm_create_from_group to create a new communicator without the
   * need to specify a parent communicator
   */
  err = MPI_Comm_create_from_group(mygroup, NULL, MPI_INFO_NULL,
                                   MPI_ERRORS_ARE_FATAL, &mycomm);
  if (err) {
    printf("Something went wrong\n");
    return err;
  }

  /*
   * Now, you can use normal MPI functions for communication
   */
  MPI_Barrier(mycomm);

  MPI_Comm_rank(mycomm, &myrank);
  MPI_Comm_size(mycomm, &size);

  printf("My rank is %d and the initial pset contains %d computing resources\n",
         myrank, size);

  if (myrank == 0) {
    strcpy(buf, HELLO_STR);
  }

  MPI_Bcast(buf, sizeof(HELLO_STR), MPI_CHAR, 0, mycomm);

  switch (myrank) {
    case 0:
      printf("I, rank %d, sent the following string: %s\n", myrank, buf);
      break;
    default:
      printf("I, rank %d, received the following string: %s\n", myrank, buf);
      break;
  }

  /*
   * Free the group & communicator, after you are done!
   */
  MPI_Group_free(&mygroup);
  MPI_Comm_free(&mycomm);

  err = MPI_Session_finalize(&mysession);
  if (err) {
    printf("Something went wrong\n");
    return err;
  }

  return EXIT_SUCCESS;
}

int main(int argc, char *argv[static argc + 1]) {
  MPI_Info manager_config;
  int world_size;
  char buf[0x20];

  MPI_Init(&argc, &argv);

  MPI_Comm_size(MPI_COMM_WORLD, &world_size);

  // manager dependent options
  MPI_Info_create(&manager_config);
  snprintf(buf, 0x20, "%d", world_size - 1);
  MPI_Info_set(manager_config, "manager_initial_number", buf);

  MPIDYNRES_SIM_config my_running_config = {
      .base_communicator = MPI_COMM_WORLD,  // simulate on MPI_COMM_WORLD
      .manager_config = manager_config,
  };

  // This call will start the simulation immediatly (and block until the
  // simualtion has finished)
  MPIDYNRES_SIM_start(my_running_config, argc, argv, MPIDYNRES_main);

  MPI_Info_free(&manager_config);

  MPI_Finalize();
}
