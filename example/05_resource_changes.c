/*
 * This example shows how to fetch new resource changes and a simple
 * communication scheme to synchronize all running computing resources
 */
#include <mpidynres.h>
#include <mpidynres_pset.h>
#include <mpidynres_sim.h>
#include <mpi.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

extern void MPIDYNRES_print_set(MPIDYNRES_pset *set);

/*
 * For synchronizing computing resources, communication is needed
 * In the following enums there are some tags defined to ease the communicatio
 */
enum { TAG_COMMAND, TAG_SHUTDOWN_ALMOST_DONE, TAG_NEW_URI };

/*
 * possible values of the TAG_COMMAND int values
 */
enum {
  NEW_CRS,
  SUB_CRS,
  NONE,
};

struct run_state {
  MPIDYNRES_init_info init_info;
  MPI_Comm running_comm;
  char running_uri[MPIDYNRES_URI_MAX_SIZE];
};

// broadcast an uri on comm by rank 0
void bcast_uri(MPI_Comm comm, char *uri) {
  printf("Broadcasting uri\n");
  MPI_Bcast(uri, MPIDYNRES_URI_MAX_SIZE, MPI_CHAR, 0, comm);
  printf("Done broadcasting uri\n");
}

/*
 * Check for resource changes
 */
void resource_changes_check(struct run_state *state) {
  MPIDYNRES_RC_type rc_type;  // there can be several
  int rc_tag;              // identifying tag of resource change
  char diff_uri[MPIDYNRES_URI_MAX_SIZE] = {0};  // uri of new/to remove resources
  char union_uri[MPIDYNRES_URI_MAX_SIZE] = {0};
  char new_running_uri[MPIDYNRES_URI_MAX_SIZE] = {0};

  int unused;
  size_t size;
  int command_type;
  MPIDYNRES_pset *set;

  /*
   * MPIDYNRES_RC_fetch is used to check for new resource changes
   * If you are done with cleanup, MPIDYNRES_RC_accept with the same tag can be
   * called to notify mpidynres that you the changes should be applied
   */
  MPIDYNRES_RC_fetch(&rc_type, diff_uri, &rc_tag);

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
  case MPIDYNRES_REPLACE:
    fprintf(stderr, "No support for REPLACE");
      exit(-1);
      break;
  }

  // send command
  MPI_Bcast(&command_type, 1, MPI_INT, 0, state->running_comm);

  switch (rc_type) {
    case MPIDYNRES_RC_ADD: {
      MPIDYNRES

      MPIDYNRES_URI_size(state->running_uri, &size);
      // create a new uri with the current and the new crs
      MPIDYNRES_URI_create_op(diff_uri, state->running_uri, MPIDYNRES_URI_UNION,
                           union_uri);
      bcast_uri(state->running_comm, union_uri);

      /*
       * MPIDYNRES_RC_accept is called after doing cleanup work to apply the
       * suggested changes
       * Here, 0 (anything other than MPIDYNRES_TAG_FIRST START works) is passed
       * and the union uri
       */
      printf("Accepting rc with tag %d\n", rc_tag);
      MPIDYNRES_RC_accept(rc_tag, 0, union_uri);

      strcpy(state->running_uri, union_uri);
      MPIDYNRES_Comm_create_uri(union_uri, &state->running_comm);

      MPIDYNRES_pset_destroy(set);
      break;
    }
    case MPIDYNRES_RC_SUB: {
      MPIDYNRES
      printf("\n\n");
      printf("TO REMOVE SET:");
      MPIDYNRES_print_set(set);
      printf("\n\n");

      MPIDYNRES_URI_size(state->running_uri, &size);

      // create a new uri with the crs removed
      MPIDYNRES_URI_create_op(state->running_uri, diff_uri, MPIDYNRES_URI_SUBTRACT,
                           new_running_uri);
      bcast_uri(state->running_comm, new_running_uri);

      // wait for exit messages of crs in diff uri
      int to_wait = set->size;
      if (MPIDYNRES_pset_contains(set, state->init_info.cr_id)) {
        to_wait -= 1;
      }
      for (int i = 0; i < to_wait; i++) {
        MPI_Recv(&unused, 1, MPI_INT, MPI_ANY_SOURCE, TAG_SHUTDOWN_ALMOST_DONE,
                 state->running_comm, MPI_STATUS_IGNORE);
      }

      /*
       * MPIDYNRES_RC_accept is called after doing cleanup work to apply the
       * suggested changes
       * Here, 0 (anything other than MPIDYNRES_TAG_FIRST START works) is passed
       * and the union uri
       */
      MPIDYNRES_RC_accept(rc_tag, 0, new_running_uri);

      // exit if not in running pset

      MPIDYNRES_pset_destroy(set);
      MPIDYNRES_URI_lookup(new_running_uri, &set);

      if (!MPIDYNRES_pset_contains(set, state->init_info.cr_id)) {
        MPIDYNRES_print_set(set);
        MPIDYNRES_pset_destroy(set);
        MPIDYNRES_exit();
      } else {
        strcpy(state->running_uri, new_running_uri);
        MPIDYNRES_Comm_create_uri(new_running_uri, &state->running_comm);
      }
      MPIDYNRES_pset_destroy(set);
      break;
    }
    case MPIDYNRES_RC_REPLACE: {
      fprintf(stderr, "No support for MPIDYNRES
      exit(-1);
      break;
    }
    case MPIDYNRES_RC_NONE: {
      break;
    }
  }
}

/*
 * This function simulated work with non-synchronous communication
 */
void do_work(struct run_state *state) {
  int unused;
  int myrank;
  int command;
  MPIDYNRES

  MPI_Comm_rank(state->running_comm, &myrank);

  while (1) {
    if (myrank == 0) {
      printf("I am cr_id %d, resource check\n", state->init_info.cr_id);
      resource_changes_check(state);

      MPI_Comm_rank(state->running_comm, &myrank);
    } else {
      // get command
      MPI_Bcast(&command, 1, MPI_INT, 0, state->running_comm);
      if (command != NONE) {
        printf("I am cr_id %d got command\n", state->init_info.cr_id);
        bcast_uri(state->running_comm, state->running_uri);

        if (command == SUB_CRS) {
          printf("I am cr_id %d sub command\n", state->init_info.cr_id);
          // check if contained in new set
          MPIDYNRES_URI_lookup(state->running_uri, &set);

          if (!MPIDYNRES_pset_contains(set, state->init_info.cr_id)) {
            printf("I am cr_id %d shutting down\n", state->init_info.cr_id);
            MPIDYNRES_pset_destroy(set);
            // shutdown
            MPI_Send(&unused, 1, MPI_INT, 0, TAG_SHUTDOWN_ALMOST_DONE,
                     state->running_comm);
            printf("Hallo\n");
            MPIDYNRES_exit();
          }
          MPIDYNRES_pset_destroy(set);
        }

        // adopt new uri
        /* int err = */
        printf("I am cr_id %d creating new comm\n", state->init_info.cr_id);
        MPIDYNRES_Comm_create_uri(state->running_uri, &state->running_comm);
        printf("I am cr_id %d done creating new comm\n", state->init_info.cr_id);
        /* if (state->running_comm == MPI_COMM_NULL) { */
        /*   MPIDYNRES_pset *set; */
        /*   MPIDYNRES_URI_lookup(state->running_uri, &set); */
        /*   MPIDYNRES_print_set(set); */
        /*   printf("%d\n", state->init_info.cr_id); */
        /*   printf("Something bad happened2\n"); */
        /*   MPIDYNRES_exit(); */
        /* } */
        /* if (err != 0) { */
        /*   printf("Something bad happened\n"); */
        /*   MPIDYNRES_exit(); */
        /* } */
        MPI_Comm_rank(state->running_comm, &myrank);
      }
    }

    // real work should happen here
    // here we just sleep some random time
    struct timespec t;
    clock_gettime(CLOCK_REALTIME, &t);
    sleep(((t.tv_nsec % 20) + 1) / 10.0);
    /* sleep(0.1); */
  }
}

int MPIDYNRES_main(int argc, char *argv[]) {
  struct run_state state_stack;
  struct run_state *state = &state_stack;

  (void)argc, (void)argv;

  MPIDYNRES_init_info_get(&state->init_info);

  /*
   * This time we have the situation that a process can also be started
   * dynamically you can switch case on the init_tag field to find out whether
   * it's an inital start or whether the tag contains something passed from
   * previous computing resources
   */
  switch (state->init_info.init_tag) {
    case MPIDYNRES_TAG_FIRST_START: {
      // setup state with inital uri
      strcpy(state->running_uri, state->init_info.uri_init);
      MPIDYNRES
      break;
    }
    default: {
      // setup state with passed uri
      strcpy(state->running_uri, state->init_info.uri_passed);
      MPIDYNRES_Comm_create_uri(state->running_uri, &state->running_comm);
      break;
    }
  }

  printf("I am cr_id %d, starting do work\n", state->init_info.cr_id);
  do_work(state);

  return EXIT_SUCCESS;
}

int main(int argc, char *argv[]) {
  int world_size;

  MPI_Init(&argc, &argv);
  MPI_Comm_size(MPI_COMM_WORLD, &world_size);
  int seed = time(NULL);
  if (argc == 2) {
    seed = atoi(argv[1]);
  }
  printf("seed: %d\n", seed);
  srand(seed);

  MPIDYNRESSIM_config my_running_config = {
      .base_communicator = MPI_COMM_WORLD,
      .num_init_crs = 1,
      //.scheduling_mode = MPIDYNRES_MODE_INC_DEC,
      .scheduling_mode = MPIDYNRES_MODE_RANDOM_DIFF,
      .change_prob = 1.0,
      .inc_dec_config =
          {
              .one_round_only = true,
          },
  };

  MPIDYNRESSIM_start_sim(my_running_config, argc, argv, MPIDYNRES_main);

  MPI_Finalize();
}
