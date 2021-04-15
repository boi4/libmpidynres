/*
 * This example shows how to use set operations to create new psets
 */
#include <mpi.h>
#include <mpidynres.h>
#include <mpidynres_sim.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// helper to print mpi info object
void print_mpi_info(MPI_Info info) {
  char key[MPI_MAX_INFO_KEY + 1] = {0};
  int nkeys, vlen, unused;
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

// print process set information
int print_pset(MPI_Session session, char *pset_name) {
  int err;
  MPI_Info info;
  err = MPI_Session_get_pset_info(session, pset_name, &info);
  if (err) {
    printf("Something went wrong\n");
    return err;
  }
  printf("Info about pset %s:\n", pset_name);
  printf("========================");
  print_mpi_info(info);
  MPI_Info_free(&info);
  return 0;
}

// THis function is used to create a communicator from a process set name
int get_comm(MPI_Session mysession, char *name, MPI_Comm *mycomm) {
  MPI_Group mygroup;
  int err;
  err = MPI_Group_from_session_pset(mysession, name, &mygroup);
  if (err) {
    printf("Something went wrong\n");
    return err;
  }
  err = MPI_Comm_create_from_group(mygroup, NULL, MPI_INFO_NULL,
                                   MPI_ERRORS_ARE_FATAL, mycomm);
  if (err) {
    printf("Something went wrong\n");
    return err;
  }
  MPI_Group_free(&mygroup);
  return 0;
}

int MPIDYNRES_main(int argc, char *argv[]) {
  MPI_Session mysession;
  MPI_Comm mycomm;
  char new_pset_name[MPI_MAX_PSET_NAME_LEN];
  int err;

  (void)argc, (void)argv;

  err = MPI_Session_init(MPI_INFO_NULL, MPI_ERRORS_ARE_FATAL, &mysession);
  if (err) {
    printf("Something went wrong\n");
    return err;
  }

  // create communicator from mpi://WORLD
  err = get_comm(mysession, "mpi://WORLD", &mycomm);
  if (err) {
    printf("Something went wrong\n");
    return err;
  }

  MPI_Barrier(mycomm);

  print_pset(mysession, "mpi://WORLD");

  // Use the "difference" operation to create a new process set that contains
  // all processes except for you
  err = MPIDYNRES_pset_create_op(mysession, MPI_INFO_NULL, "mpi://WORLD",
                                 "mpi://SELF", MPIDYNRES_PSET_DIFFERENCE,
                                 new_pset_name);
  if (err) {
    printf("Something went wrong\n");
    return err;
  }

  if (new_pset_name[0] == '0') {
    printf("Something went wrong2\n");
    return err;
  }

  print_pset(mysession, new_pset_name);

  MPI_Barrier(mycomm);

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
