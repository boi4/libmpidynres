/*
 * This example shows how to fetch new resource changes and a simple
 * communication scheme to synchronize all running computing resources
 * Please check the Thesis for an explanation of this example
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

MPI_Session session;  // session object
MPI_Comm main_comm;   // the communicator where all the work happens
int main_rank;        // rank in main_comm
int main_iter;        // current application iteration
char main_pset[MPI_MAX_PSET_NAME_LEN];  // the pset that main_comm is based on

bool info_contains(MPI_Info info, const char *key) {
  int contains, unused;
  if (info == MPI_INFO_NULL) {
    return false;
  }
  MPI_Info_get_valuelen(info, key, &unused, &contains);
  return contains;
}

void update_main_comm() {
  MPI_Group mygroup;
  MPI_Group_from_session_pset(session, main_pset, &mygroup);
  MPI_Comm_create_from_group(mygroup, NULL, MPI_INFO_NULL, MPI_ERRORS_ARE_FATAL,
                             &main_comm);
  MPI_Comm_rank(main_comm, &main_rank);
  MPI_Group_free(&mygroup);
}

void fetch_resource_changes(int *rc_tag, MPIDYNRES_RC_type *rc_type,
                            char delta_pset[]) {
  MPI_Info rc_info;

  if (main_rank == 0) {
    MPIDYNRES_RC_get(session, rc_type, delta_pset, rc_tag, &rc_info);
    if (rc_info != MPI_INFO_NULL) {
      MPI_Info_free(&rc_info);
    }
  }
  MPI_Bcast(rc_type, 1, MPI_INT, 0, main_comm);
  MPI_Bcast(rc_tag, 1, MPI_INT, 0, main_comm);
  MPI_Bcast(delta_pset, MPI_MAX_PSET_NAME_LEN, MPI_CHAR, 0, main_comm);
}

void handle_resource_changes(MPIDYNRES_RC_type rc_type, char delta_pset[],
                             bool *need_to_break) {
  MPI_Info mypsets;

  switch (rc_type) {
    case MPIDYNRES_RC_ADD: {
      if (main_rank == 0) {
        MPIDYNRES_pset_create_op(session, MPI_INFO_NULL, main_pset, delta_pset, MPIDYNRES_PSET_UNION, main_pset);
      }
      MPI_Bcast(main_pset, MPI_MAX_PSET_NAME_LEN, MPI_CHAR, 0, main_comm);
      break;
    }
    case MPIDYNRES_RC_SUB: {
      if (main_rank == 0) {
        MPIDYNRES_pset_create_op(session, MPI_INFO_NULL, main_pset, delta_pset, MPIDYNRES_PSET_DIFFERENCE, main_pset);
      }
      MPI_Bcast(main_pset, MPI_MAX_PSET_NAME_LEN, MPI_CHAR, 0, main_comm);

      MPI_Session_get_psets(session, MPI_INFO_NULL, &mypsets);
      if (!info_contains(mypsets, main_pset)) {
        *need_to_break = true;
      }
      MPI_Info_free(&mypsets);
      break;
    }
    default: {
      break;
    }
  }
}

void accept_resource_change(MPIDYNRES_RC_tag rc_tag) {
  if (main_rank == 0) {
    MPI_Info new_processes_info;
    char buf[0x20];

    MPI_Info_create(&new_processes_info);
    MPI_Info_set(new_processes_info, "ex7://main_pset", main_pset);
    snprintf(buf, 0x1f, "%d\n", main_iter);
    MPI_Info_set(new_processes_info, "ex7://main_iter", buf);

    MPIDYNRES_RC_accept(session, rc_tag, new_processes_info);

    MPI_Info_free(&new_processes_info);
  }
}

void resource_changes_step(bool *need_to_break) {
  int rc_tag;
  MPIDYNRES_RC_type rc_type;
  char delta_pset[MPI_MAX_PSET_NAME_LEN];

  MPI_Barrier(main_comm);
  fetch_resource_changes(&rc_tag, &rc_type, delta_pset);
  if (rc_type != MPIDYNRES_RC_NONE) {
    printf("rc_type %d\n", rc_type);
    handle_resource_changes(rc_type, delta_pset, need_to_break);
    accept_resource_change(rc_tag);
    if (!*need_to_break) {
      update_main_comm();
    }
  }
}

void work_step() {
  // real work should happen here
  struct timespec t;
  clock_gettime(CLOCK_REALTIME, &t);
  usleep(((t.tv_nsec % 20) + 1) * 100000);  // sleep between 0.1 and 2.0 seconds
}

void rebalance_step() {
  // specific application rebalance happens here
}

void initialization_phase() {
  MPI_Info session_info, psets;
  int unused;
  char main_iter_buf[0x20];

  MPI_Session_init(MPI_INFO_NULL, MPI_ERRORS_ARE_FATAL, &session);

  MPI_Session_get_psets(session, MPI_INFO_NULL, &psets);

  if (info_contains(psets, "mpi://WORLD")) {
    strcpy(main_pset, "mpi://WORLD");
    main_iter = 0;

    // application specific initialization here

  } else {
    MPI_Session_get_info(session, &session_info);

    MPI_Info_get(session_info, "ex7://main_pset", MPI_MAX_PSET_NAME_LEN - 1,
                 main_pset, &unused);
    MPI_Info_get(session_info, "ex7://main_iter", 0x20 - 1, main_iter_buf,
                 &unused);
    main_iter = atoi(main_iter_buf);
    MPI_Info_free(&session_info);
  }

  // create main_comm from main_pset
  update_main_comm();

  MPI_Info_free(&psets);
}

void finalization_phase() {
  // application specific cleanup here

  // cleanup
  MPI_Comm_free(&main_comm);
  MPI_Session_finalize(&session);
}

int MPIDYNRES_main(int argc, char *argv[]) {
  (void)argc, (void)argv;
  bool need_to_break = false;

  initialization_phase();

  for (; main_iter < 1000 && !need_to_break; main_iter++) {
    work_step();
    resource_changes_step(&need_to_break);
    rebalance_step();
    printf("%s %d %d\n", main_pset, main_iter, need_to_break);
  }

  finalization_phase();

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
