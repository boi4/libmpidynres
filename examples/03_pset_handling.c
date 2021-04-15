/*
 * An example on how to use pset in libmpidynres and how to get more information
 * about the resources contained in a pset
 */
#include <mpi.h>
#include <mpidynres.h>
#include <mpidynres_sim.h>
#include <stdio.h>
#include <stdlib.h>



// a small helper function to print an MPI info object
void print_mpi_info(MPI_Info info) {
  char key[MPI_MAX_INFO_KEY + 1];
  int nkeys, vlen, unused;
  MPI_Info_get_nkeys(info, &nkeys);
  printf("\nMPI INFO\n");
  printf("===================\n");
  printf("{\n");
  for (int i = 0; i < nkeys; i++) {
    MPI_Info_get_nthkey(info, i, key);
    MPI_Info_get_valuelen(info, key, &vlen, &unused);
    char *val = calloc(1, vlen);
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


// get and print process set information
int print_pset_info(MPI_Session mysession) {
  int err, num_psets;
  MPI_Info info, psets_info;
  char key[MPI_MAX_INFO_KEY + 1];

  // Get all available process sets
  err = MPI_Session_get_psets(mysession, MPI_INFO_NULL, &psets_info);
  if (err) {
    printf("Something went wrong\n");
    return err;
  }

  // loop over all process sets
  MPI_Info_get_nkeys(psets_info, &num_psets);
  printf("Number of psets containing me: %d\n", num_psets);

  for (int i = 0; i < num_psets; i++) {
    MPI_Info_get_nthkey(psets_info, i, key);
    printf("pset number %d is called %s\n", i, key);

    // get and print process set info
    err = MPI_Session_get_pset_info(mysession, key, &info);
    if (err) {
      printf("Something went wrong\n");
      return err;
    }
    if (info == MPI_INFO_NULL) {
      printf("Failed to find a pset called %s\n", key);
      printf("Did one of the process members exit already?\n");
    } else {
      printf("info about pset %s:\n", key);
      print_mpi_info(info);
      // don't forget to free the info object
      MPI_Info_free(&info);
    }
  }

  return 0;
}

int print_session_info(MPI_Session mysession) {
  int err;
  MPI_Info info;

  printf("Session info:\n");
  printf("===========================\n");
  err = MPI_Session_get_info(mysession, &info);
  if (err) {
    printf("Something went wrong\n");
    return err;
  }

  if (info == MPI_INFO_NULL) {
    printf("This session contains no information\n");
  } else {
    print_mpi_info(info);
  }
  MPI_Info_free(&info);
  printf("===========================\n");
  printf("\n\n\n");

  return 0;
}

int MPIDYNRES_main(int argc, char *argv[]) {
  (void)argc, (void)argv;
  MPI_Session mysession;
  int err;

  err = MPI_Session_init(MPI_INFO_NULL, MPI_ERRORS_ARE_FATAL, &mysession);
  if (err) {
    printf("Something went wrong\n");
    return err;
  }


  err = print_pset_info(mysession);
  if (err) {
    printf("Something went wrong\n");
    return err;
  }

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
