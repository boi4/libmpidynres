/*
 * This example shows how to fetch new resource changes and a simple
 * communication scheme to synchronize all running computing resources
 */
#include <mpi.h>
#include <mpidynres.h>
#include <mpidynres_sim.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

struct run_state {
  MPI_Session session;
  int process_id;
  MPI_Info info;
  MPI_Comm running_comm;  // the communicator where all the work happens
  int running_rank;
  char running_pset[MPI_MAX_PSET_NAME_LEN];  // the pset that running_comm is
};

void print_mpi_info(MPI_Info info) {
  char key[MPI_MAX_INFO_KEY + 1];
  int nkeys, vlen, unused;
  if (info == MPI_INFO_NULL) {
    printf("INFO NULL\n\n\n");
    return;
  }
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

int get_comm(MPI_Session mysession, char *name, MPI_Comm *mycomm) {
  MPI_Group mygroup;
  int err;

  printf("Get comm %s\n", name);

  err = MPI_Group_from_session_pset(mysession, name, &mygroup);
  if (err) {
    printf("Something went wrong\n");
    return err;
  }
  printf("Got group, now creating comm\n");
  err = MPI_Comm_create_from_group(mygroup, NULL, MPI_INFO_NULL,
                                   MPI_ERRORS_ARE_FATAL, mycomm);
  if (err) {
    printf("Something went wrong\n");
    return err;
  }
  MPI_Group_free(&mygroup);
  return 0;
}

// broadcast a pset name on comm by rank 0
void bcast_pset_name(MPI_Comm comm, char *pset_name) {
  printf("Broadcasting pset name\n");
  MPI_Bcast(pset_name, MPI_MAX_PSET_NAME_LEN, MPI_CHAR, 0, comm);
  printf("Done broadcasting pset name\n");
}

void accept_rc(struct run_state *state, int rc_tag) {
  MPI_Info info;
  MPI_Info_create(&info);
  MPI_Info_set(info, "state_running_pset", state->running_pset);
  printf("Accepting rc\n");
  MPIDYNRES_RC_accept(state->session, rc_tag, info);
  MPI_Info_free(&info);
}




/*
 * Check for resource changes
 *
 * Returns whether this process has to break the loop
 */
bool resource_changes_check(struct run_state *state) {
  int rc_tag;  // identifying tag of resource change
  int unused;
  int contains_me;
  char diff_pset[MPI_MAX_PSET_NAME_LEN] = {0};
  char new_running_pset[MPI_MAX_PSET_NAME_LEN] = {0};
  MPI_Info psets_info;
  MPI_Info rc_info;
  MPIDYNRES_RC_type rc_type;

  printf("In resource changes check\n");

  /*
   * MPIDYNRES_RC_fetch is used to check for new resource changes
   * If you are done with cleanup, MPIDYNRES_RC_accept with the same tag can be
   * called to notify mpidynres that you the changes should be applied
   */
  if (state->running_rank == 0) {
    MPIDYNRES_RC_fetch(state->session, &rc_type, diff_pset, &rc_tag, &rc_info);
    printf("RC INFO:\n");
    print_mpi_info(rc_info);
    if (rc_info != MPI_INFO_NULL) {
      MPI_Info_free(&rc_info);
    }
  }
  printf("Hallo\n");
  // MPIDYNRES_RC_Type is an enum type
  MPI_Bcast(&rc_type, 1, MPI_INT, 0, state->running_comm);

  switch (rc_type) {
    case MPIDYNRES_RC_ADD: {
      if (state->running_rank == 0) {
        printf("New Resources will be added under %s\n", diff_pset);
        MPIDYNRES_pset_create_op(state->session, MPI_INFO_NULL,
                                 state->running_pset, diff_pset,
                                 MPIDYNRES_PSET_UNION, new_running_pset);
      }
      bcast_pset_name(state->running_comm, new_running_pset);
      strcpy(state->running_pset, new_running_pset);
      if (state->running_rank == 0) {
        accept_rc(state, rc_tag);
      }

      // update state
      MPI_Comm_free(&state->running_comm);
      get_comm(state->session, state->running_pset, &state->running_comm);
      MPI_Comm_rank(state->running_comm, &state->running_rank);
      break;
    }

    case MPIDYNRES_RC_SUB: {
      if (state->running_rank == 0) {
        MPIDYNRES_pset_create_op(state->session, MPI_INFO_NULL,
                                 state->running_pset, diff_pset,
                                 MPIDYNRES_PSET_DIFFERENCE, new_running_pset);
        printf("Resources will be removed be added under %s\n", diff_pset);
      }
      bcast_pset_name(state->running_comm, new_running_pset);

      // check if new pset contains this process
      MPI_Session_get_psets(state->session, MPI_INFO_NULL, &psets_info);
      MPI_Info_get_valuelen(psets_info, new_running_pset, &unused,
                            &contains_me);
      MPI_Info_free(&psets_info);

      if (!contains_me) {
        printf("%d: New Process doesn't contain me, need to shutdown\n", state->process_id);
        return true;
      }

      strcpy(state->running_pset, new_running_pset);
      if (state->running_rank == 0) {
        accept_rc(state, rc_tag);
      }

      // update state
      MPI_Comm_free(&state->running_comm);
      get_comm(state->session, state->running_pset, &state->running_comm);
      MPI_Comm_rank(state->running_comm, &state->running_rank);
      break;
    }

    case MPIDYNRES_RC_NONE: {
      break;
    }

    case MPIDYNRES_RC_REPLACE: {
      fprintf(stderr, "No support for REPLACE");
      exit(-1);
      break;
    }
  }

  return false;
}

/*
 * This function simulated work with non-synchronous communication
 */
void do_work(struct run_state *state) {
  bool need_to_return;

  while (true) {
    MPI_Barrier(state->running_comm);

    need_to_return = resource_changes_check(state);
    if (need_to_return) {
      break;
    }

    printf("New round on pset %s\n", state->running_pset);

    // real work should happen here
    // here we just sleep some random time
    struct timespec t;
    clock_gettime(CLOCK_REALTIME, &t);
    sleep(((t.tv_nsec % 20) + 1) / 10.0);
  }
}

int MPIDYNRES_main(int argc, char *argv[]) {
  struct run_state state_stack;
  struct run_state *state = &state_stack;
  char buf[0x20];
  int in_there;

  (void)argc, (void)argv;

  // intitialize
  MPI_Session_init(MPI_INFO_NULL, MPI_ERRORS_ARE_FATAL, &state->session);

  MPI_Session_get_info(state->session, &state->info);

  MPI_Info_get(state->info, "mpidynres_process_id", 0x1f, buf, &in_there);
  if (!in_there) {
    fprintf(stderr, "Error: Expected mpidynres_process_id in session info\n");
    exit(1);
  }
  state->process_id = atoi(buf);

  MPI_Info_get(state->info, "state_running_pset", MPI_MAX_PSET_NAME_LEN - 1,
               state->running_pset, &in_there);

  if (!in_there) {
    // This means that this is the initial start
    printf("This is the initial start, using mpidynres://INIT as main pset\n");
    strcpy(state->running_pset, "mpidynres://INIT");
  }

  get_comm(state->session, state->running_pset, &state->running_comm);
  MPI_Comm_rank(state->running_comm, &state->running_rank);

  // start loop
  printf("I am process %d, starting do work\n", state->process_id);
  do_work(state);

  // cleanup
  MPI_Comm_free(&state->running_comm);
  MPI_Info_free(&state->info);
  MPI_Session_finalize(&state->session);

  return EXIT_SUCCESS;
}

int main(int argc, char *argv[static argc + 1]) {
  MPI_Info manager_config;
  int world_size;
  char buf[0x20];

  int seed = time(NULL);
  if (argc == 2) {
    seed = atoi(argv[1]);
  }
  printf("seed: %d\n", seed);
  srand(seed);

  MPI_Init(&argc, &argv);

  MPI_Comm_size(MPI_COMM_WORLD, &world_size);

  // manager dependent options
  MPI_Info_create(&manager_config);
  snprintf(buf, 0x20, "%d", world_size - 1);
  MPI_Info_set(manager_config, "manager_initial_number", buf);

  MPIDYNRESSIM_config my_running_config = {
      .base_communicator = MPI_COMM_WORLD,  // simulate on MPI_COMM_WORLD
      .manager_config = manager_config,
  };

  // This call will start the simulation immediatly (and block until the
  // simualtion has finished)
  MPIDYNRESSIM_start_sim(my_running_config, argc, argv, MPIDYNRES_main);

  MPI_Info_free(&manager_config);

  MPI_Finalize();
}
