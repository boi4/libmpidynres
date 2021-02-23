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

/*
 * For synchronizing computing resources, communication is needed
 * In the following enums there are some tags defined to ease the communicatio
 */
enum { TAG_COMMAND, TAG_SHUTDOWN_ALMOST_DONE };

/*
 * possible values of the TAG_COMMAND int values
 */
enum {
  NEW_CRS,
  SUB_CRS,
  NONE,
};

struct run_state {
  MPI_Session session;
  int process_id;
  MPI_Info info;
  MPI_Comm running_comm;  // the communicator where all the work happens
  char running_pset[MPI_MAX_PSET_NAME_LEN];  // the pset that running_comm is
                                             // based on
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
 */
int resource_changes_check(struct run_state *state) {
  MPIDYNRES_RC_type rc_type;  // there can be several
  int rc_tag;                 // identifying tag of resource change
  char diff_pset[MPI_MAX_PSET_NAME_LEN] = {
      0};  // uri of new/to remove resources
  char union_pset[MPI_MAX_PSET_NAME_LEN] = {0};
  char new_running_pset[MPI_MAX_PSET_NAME_LEN] = {0};
  char buf[0x20] = {0};
  MPI_Info pset_info;
  MPI_Info psets_info;
  MPI_Info rc_info;

  int unused;
  int command_type;
  int contains_me;

  printf("In resource changes check\n");

  /*MPIDYNRES_RC_fetch is used to check for new resource changes*/
  /*If you are done with cleanup, MPIDYNRES_RC_accept with the same tag can be*/
  /*called to notify mpidynres that you the changes should be applied*/
  MPIDYNRES_RC_fetch(state->session, &rc_type, diff_pset, &rc_tag, &rc_info);
  printf("RC INFO:\n");
  print_mpi_info(rc_info);

  switch (rc_type) {
    case MPIDYNRES_RC_ADD:
      command_type = NEW_CRS;
      break;
    case MPIDYNRES_RC_SUB:
      command_type = SUB_CRS;
      break;
    case MPIDYNRES_RC_NONE:
      command_type = NONE;
      break;
    case MPIDYNRES_RC_REPLACE:
      fprintf(stderr, "No support for REPLACE");
      exit(-1);
      break;
  }
  printf("Hallo1\n");

  // send command
  MPI_Bcast(&command_type, 1, MPI_INT, 0, state->running_comm);

  printf("Hallo2\n");

  switch (rc_type) {
    case MPIDYNRES_RC_ADD: {
      printf("Got RC_ADD\n");
      // create a new pset with the current and the new crs
      MPIDYNRES_pset_create_op(state->session, MPI_INFO_NULL,
                               state->running_pset, diff_pset,
                               MPIDYNRES_PSET_UNION, union_pset);
      bcast_pset_name(state->running_comm, union_pset);

      printf("Accepting rc with tag %d\n", rc_tag);

      strcpy(state->running_pset, union_pset);
      accept_rc(state, rc_tag);

      MPI_Comm_free(&state->running_comm);
      get_comm(state->session, state->running_pset, &state->running_comm);

      break;
    }
    case MPIDYNRES_RC_SUB: {
      printf("Got RC_SUB\n");
      // create a new pset with the crs removed
      MPIDYNRES_pset_create_op(state->session, MPI_INFO_NULL,
                               state->running_pset, diff_pset,
                               MPIDYNRES_PSET_DIFFERENCE, new_running_pset);
      printf("Hallo3\n");
      bcast_pset_name(state->running_comm, new_running_pset);
      printf("Hallo4\n");

      MPI_Session_get_pset_info(state->session, new_running_pset, &pset_info);
      MPI_Info_get(pset_info, "size", 0x1f, buf, &unused);
      // wait for exit messages of crs in diff uri
      int to_wait = atoi(buf);
      MPI_Info_free(&pset_info);

      MPI_Session_get_psets(state->session, MPI_INFO_NULL, &psets_info);
      MPI_Info_get_valuelen(psets_info, new_running_pset, &unused,
                            &contains_me);
      MPI_Info_free(&psets_info);

      if (contains_me) {
        to_wait -= 1;
      }
      for (int i = 0; i < to_wait; i++) {
        MPI_Recv(&unused, 1, MPI_INT, MPI_ANY_SOURCE, TAG_SHUTDOWN_ALMOST_DONE,
                 state->running_comm, MPI_STATUS_IGNORE);
      }

      strcpy(state->running_pset, new_running_pset);
      accept_rc(state, rc_tag);

      // exit if not in running pset

      if (!contains_me) {
        return 1;  // need to return from do_work
      } else {
        // adopt new comm
        MPI_Comm_free(&state->running_comm);
        get_comm(state->session, state->running_pset, &state->running_comm);
      }
      break;
    }
    case MPIDYNRES_RC_REPLACE: {
      fprintf(stderr, "TODO");
      exit(-1);
      break;
    }
    case MPIDYNRES_RC_NONE: {
      printf("Got RC_NONE\n");
      break;
    }
  }
  return 0;
}

/*
 * This function simulated work with non-synchronous communication
 */
void do_work(struct run_state *state) {
  int unused = {0};
  int myrank;
  int command;
  int in_there;
  int need_to_return;
  MPI_Info psets_info;

  MPI_Comm_rank(state->running_comm, &myrank);

  while (true) {
    MPI_Barrier(state->running_comm);

    if (myrank == 0) {
      printf("I am process %d, resource check\n", state->process_id);
      need_to_return = resource_changes_check(state);
      if (need_to_return) {
        return;
      }

      MPI_Comm_rank(state->running_comm, &myrank);
    } else {
      // get command
      MPI_Bcast(&command, 1, MPI_INT, 0, state->running_comm);

      if (command != NONE) {
        printf("I am process %d got command\n", state->process_id);
        bcast_pset_name(state->running_comm, state->running_pset);

        if (command == SUB_CRS) {
          printf("I am process %d sub command\n", state->process_id);

          MPI_Session_get_psets(state->session, MPI_INFO_NULL, &psets_info);

          MPI_Info_get_valuelen(psets_info, state->running_pset, &unused,
                                &in_there);

          if (!in_there) {
            printf("I am process %d shutting down\n", state->process_id);
            MPI_Info_free(&psets_info);
            // shutdown
            MPI_Send(&unused, 1, MPI_INT, 0, TAG_SHUTDOWN_ALMOST_DONE,
                     state->running_comm);
            return;
          }

          MPI_Info_free(&psets_info);
        }

        // adopt new uri
        printf("I am process %d creating new comm\n", state->process_id);
        MPI_Comm_free(&state->running_comm);
        get_comm(state->session, state->running_pset, &state->running_comm);
        printf("I am process %d done creating new comm\n", state->process_id);
        MPI_Comm_rank(state->running_comm, &myrank);
      }
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

  printf("I am process %d, starting do work\n", state->process_id);
  do_work(state);

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
