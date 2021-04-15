#include "scheduler_mgmt.h"
#include "util.h"
#include "scheduler_datatypes.h"

struct inc_dec_manager {
  MPIDYNRES_scheduler *scheduler;
  int num_processes;
  int next;
  bool increasing;
};
typedef struct inc_dec_manager inc_dec_manager;


/**
 * @brief      Initialize the manager
 *
 * @param      scheduler The scheduler that is using the management interface
 *
 * @return     The new manager object
 */
MPIDYNRES_manager MPIDYNRES_manager_init(MPIDYNRES_scheduler *scheduler) {
  inc_dec_manager *res;
  res = calloc(1, sizeof(inc_dec_manager));
  if (res == NULL) {
    die("Memory Error\n");
  }
  res->scheduler = scheduler;
  res->num_processes = scheduler->num_scheduling_processes;

  return res;
}


/**
 * @brief      Free a manger
 *
 * @param      manager The manager to be freed
 *
 * @return     if != 0, an error occured
 */
int MPIDYNRES_manager_free(MPIDYNRES_manager manager) {
  free(manager);
  return 0;
}


/*
 * No support yet
 */
int MPIDYNRES_manager_register_scheduling_hints(MPIDYNRES_manager manager,
                                                int src_process_id,
                                                MPI_Info scheduling_hints,
                                                MPI_Info *o_answer) {
  (void) manager;
  (void) src_process_id;
  (void) scheduling_hints;
  *o_answer = MPI_INFO_NULL;
  return 0;
}


/**
 * @brief      Get initial process set
 *
 * @details    Check for config or default to cr id 1
 *
 * @param      manager The manager used
 *
 * @param      o_initial_pset The initial pset is returned here
 *
 * @return     if != 0, an error occured
 */
int MPIDYNRES_manager_get_initial_pset(MPIDYNRES_manager manager,
                                       set_int *o_initial_pset) {
  inc_dec_manager *mgr = (inc_dec_manager *)manager;
  int vlen;
  int in_there;
  int num_init = 1;
  MPI_Info config = mgr->scheduler->config->manager_config;

  if (config == MPI_INFO_NULL) {
    in_there = false;
  } else {
    MPI_Info_get_valuelen(config, "manager_initial_number", &vlen, &in_there);
  }
  if (in_there) {
    char *value = calloc(vlen+1, sizeof(char));
    MPI_Info_get(config, "manager_initial_number", vlen, value, &in_there);
    num_init = atoi(value);
    free(value);
    if (num_init < 1 || num_init > mgr->num_processes) {
      die("Key manager_initial_number is invalid.\n");
    }
  } else {
    if (config == MPI_INFO_NULL) {
      in_there = false;
    } else {
      MPI_Info_get_valuelen(config, "manager_initial_number_random", &vlen, &in_there);
    }
    if (in_there) {
      num_init = 1 + (rand() % (mgr->num_processes - 1));
    } else {
      num_init = 1;
    }
  }

  // start processes 1...num_init
  *o_initial_pset = set_int_init(int_compare);
  for (int i = 0; i < num_init; i++) {
    set_int_insert(o_initial_pset, i+1);
  }

  if (num_init == mgr->num_processes) {
    mgr->next = mgr->num_processes;
    mgr->increasing = false;
  } else {
    mgr->next = mgr->num_processes + 1;
    mgr->increasing = true;
  }

  return 0;
}


/**
 * @brief      Handle a resource change query
 *
 * @param      manager The manager used
 *
 * @param      src_process_id The cr id of the calling computing resource
 *
 * @param      o_rc_info The resource change info that should be returned to the application
 *
 * @param      o_rc_type The type of resource change is returned here
 *
 * @param      o_new_pset The new process set is returned here
 *
 * @return     if != 0, an error occured
 */
int MPIDYNRES_manager_handle_rc_msg(MPIDYNRES_manager manager,
                                    int src_process_id,
                                    MPI_Info *o_rc_info,
                                    MPIDYNRES_RC_type *o_rc_type,
                                    set_int *o_new_pset) {
  (void) src_process_id;
  inc_dec_manager *mgr = (inc_dec_manager *)manager;

  if (mgr->increasing) {
    *o_rc_type = MPIDYNRES_RC_ADD;
  } else {
    *o_rc_type = MPIDYNRES_RC_SUB;
  }

  *o_new_pset = set_int_init(int_compare);
  set_int_insert(o_new_pset, mgr->next);

  // update next
  if (mgr->increasing && mgr->next == mgr->num_processes) {
    mgr->next = mgr->num_processes;
    mgr->increasing = false;
  } else if (!mgr->increasing && mgr->next == 2){
    mgr->next = 2;
    mgr->increasing = true;
  } else {
    mgr->next += (mgr->increasing ? 1 : -1);
  }

  *o_rc_info = MPI_INFO_NULL;

  return 0;
}
