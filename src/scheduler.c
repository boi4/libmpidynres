#include "scheduler.h"
// TODO: Improve error handling

#include <mpi.h>
#include <stdlib.h>
#include <string.h>

#include "comm.h"
#include "logging.h"
#include "mpidynres.h"
#include "scheduler_handlers.h"
#include "scheduling_modes.h"
#include "util.h"

/*
 * PRIVATE FUNCTIONS
 */
/**
 * @brief      send the "start" command to a cr
 *
 * @param      scheduler the scheduler
 *
 *
 * @return     return type
 */
void MPIDYNRES_scheduler_start_cr(MPIDYNRES_scheduler *scheduler, int i_cr, bool dynamic_start, int origin_rc_tag) {
  MPIDYNRES_idle_command command = {
    .command_type = start,
  };

  // check that process is neither running nor reserved
  set_process_state_node *res = set_process_state_find(&scheduler->process_states,
                                                       (process_state){.process_id = i_cr});
  assert(res == NULL);
  process_state new_process_state = {
    .process_id = i_cr,
    .active = true,
    .reserved = false,
    .pending_shutdown = false,
    .dynamic_start = dynamic_start,
    .origin_rc_tag = origin_rc_tag,
  };
  set_process_state_insert(&scheduler->process_states, new_process_state);

  // check that it's contained in running_crs
  assert(set_int_count(&scheduler->running_crs, i_cr) == 1);

  MPI_Send(&command, 1, get_idle_command_datatype(), i_cr,
           MPIDYNRES_TAG_IDLE_COMMAND, scheduler->config->base_communicator);
  set_state(i_cr, running);
  log_state("Starting cr id %d", i_cr);
}

/**
 * @brief      Send shutdown signal to all computing resources
 *
 * @param      scheduler the scheduler
 */
void MPIDYNRES_scheduler_shutdown_all_crs(MPIDYNRES_scheduler *scheduler) {
  MPIDYNRES_idle_command command = {
    .command_type = shutdown,
  };

  assert(scheduler->running_crs.size == 0);
  // TODO: is there a send to all function?
  // TODO: is there more needed here?
  for (int cr = 1; cr < 1 + scheduler->num_scheduling_processes; cr++) {
    MPI_Send(&command, 1, get_idle_command_datatype(), cr,
    MPIDYNRES_TAG_IDLE_COMMAND, scheduler->config->base_communicator);
  }
}

/**
 * @brief      the main loop of the scheduler
 *
 * @details    the most important function of the scheduler, it is waiting for
 * different requests, starts the handler and gets back to waiting. when all crs
 * are idle, it will shut them all down and return
 * For the handlers themselves, see scheduler_
 *
 * @param      scheduler the scheduler
 */
void MPIDYNRES_scheduler_schedule(MPIDYNRES_scheduler *scheduler) {
  MPI_Status status;
  int index;

  // different message contents
  int unused = {0};
  int session_id = {0};
  int pset_info_msg[2] = {0};
  int pset_lookup_msg[2] = {0};
  MPIDYNRES_RC_accept_msg rc_accept_msg = {0};
  MPIDYNRES_pset_op_msg pset_op_msg = {0};
  MPIDYNRES_pset_free_msg pset_free_msg = {0};

  // different requests that we could get
  enum {
    worker_done_request,

    session_create_request,
    session_info_request,
    session_finalize_request,

    get_psets_request,
    pset_info_request,

    pset_lookup_request,
    pset_op_request,
    pset_free_request,

    rc_request,
    rc_accept_request,

    REQUEST_COUNT
  };

  MPI_Request requests[REQUEST_COUNT];

  // setup requests array
  MPI_Irecv(&unused, 1, MPI_INT, MPI_ANY_SOURCE, MPIDYNRES_TAG_DONE_RUNNING,
            scheduler->config->base_communicator,
            &requests[worker_done_request]);

  MPI_Irecv(&unused, 1, MPI_INT, MPI_ANY_SOURCE, MPIDYNRES_TAG_SESSION_CREATE,
            scheduler->config->base_communicator,
            &requests[session_create_request]);
  MPI_Irecv(&session_id, 1, MPI_INT, MPI_ANY_SOURCE, MPIDYNRES_TAG_SESSION_INFO,
            scheduler->config->base_communicator,
            &requests[session_info_request]);
  MPI_Irecv(&session_id, 1, MPI_INT, MPI_ANY_SOURCE,
            MPIDYNRES_TAG_SESSION_FINALIZE,
            scheduler->config->base_communicator,
            &requests[session_finalize_request]);

  MPI_Irecv(&session_id, 1, MPI_INT, MPI_ANY_SOURCE, MPIDYNRES_TAG_GET_PSETS,
            scheduler->config->base_communicator, &requests[get_psets_request]);
  MPI_Irecv(pset_info_msg, 2, MPI_INT, MPI_ANY_SOURCE, MPIDYNRES_TAG_PSET_INFO,
            scheduler->config->base_communicator, &requests[pset_info_request]);

  MPI_Irecv(pset_lookup_msg, 2, MPI_INT, MPI_ANY_SOURCE,
            MPIDYNRES_TAG_PSET_LOOKUP, scheduler->config->base_communicator,
            &requests[pset_lookup_request]);
  MPI_Irecv(&pset_op_msg, 1, get_pset_op_datatype(), MPI_ANY_SOURCE,
            MPIDYNRES_TAG_PSET_OP, scheduler->config->base_communicator,
            &requests[pset_op_request]);
  MPI_Irecv(&pset_free_msg, MPI_MAX_PSET_NAME_LEN, MPI_CHAR, MPI_ANY_SOURCE,
            MPIDYNRES_TAG_PSET_FREE, scheduler->config->base_communicator,
            &requests[pset_free_request]);

  MPI_Irecv(&unused, 1, MPI_INT, MPI_ANY_SOURCE, MPIDYNRES_TAG_RC,
            scheduler->config->base_communicator, &requests[rc_request]);
  MPI_Irecv(&rc_accept_msg, 1, get_rc_datatype(), MPI_ANY_SOURCE,
            MPIDYNRES_TAG_RC_ACCEPT, scheduler->config->base_communicator,
            &requests[rc_accept_request]);

  while (scheduler->running_crs.size > 0) {
    debug("Waiting for commands...\n");

    MPI_Waitany(REQUEST_COUNT, requests, &index, &status);

    switch (index) {
      case worker_done_request: {
        MPIDYNRES_scheduler_handle_worker_done(scheduler, &status);

        // create new request
        MPI_Irecv(&unused, 1, MPI_INT, MPI_ANY_SOURCE,
                  MPIDYNRES_TAG_DONE_RUNNING,
                  scheduler->config->base_communicator,
                  &requests[worker_done_request]);
        break;
      }

      case session_create_request: {
        MPIDYNRES_scheduler_handle_session_create(scheduler, &status);

        // create new request
        MPI_Irecv(&unused, 1, MPI_INT, MPI_ANY_SOURCE,
                  MPIDYNRES_TAG_SESSION_CREATE,
                  scheduler->config->base_communicator,
                  &requests[session_create_request]);
        break;
      }
      case session_info_request: {
        MPIDYNRES_scheduler_handle_session_info(scheduler, &status);

        // create new request
        MPI_Irecv(&session_id, 1, MPI_INT, MPI_ANY_SOURCE,
                  MPIDYNRES_TAG_SESSION_INFO,
                  scheduler->config->base_communicator,
                  &requests[session_info_request]);
        break;
      }
      case session_finalize_request: {
        MPIDYNRES_scheduler_handle_session_finalize(scheduler, &status);

        // create new request
        MPI_Irecv(&session_id, 1, MPI_INT, MPI_ANY_SOURCE,
                  MPIDYNRES_TAG_SESSION_FINALIZE,
                  scheduler->config->base_communicator,
                  &requests[session_finalize_request]);

        break;
      }

      case get_psets_request: {
        MPIDYNRES_scheduler_handle_get_psets(scheduler, &status);

        // create new request
        MPI_Irecv(&session_id, 1, MPI_INT, MPI_ANY_SOURCE,
                  MPIDYNRES_TAG_GET_PSETS, scheduler->config->base_communicator,
                  &requests[get_psets_request]);
        break;
      }
      case pset_info_request: {
        debug("pset_info_msg: [%d, %d] from %d\n", pset_info_msg[0],
              pset_info_msg[1], status.MPI_SOURCE);
        MPIDYNRES_scheduler_handle_pset_info(scheduler, &status,
                                             pset_info_msg[1]);

        // create new request
        MPI_Irecv(pset_info_msg, 2, MPI_INT, MPI_ANY_SOURCE,
                  MPIDYNRES_TAG_PSET_INFO, scheduler->config->base_communicator,
                  &requests[pset_info_request]);
        break;
      }

      case pset_lookup_request: {
        MPIDYNRES_scheduler_handle_pset_lookup(scheduler, &status,
                                               pset_lookup_msg[1]);

        // create new request
        MPI_Irecv(&pset_lookup_msg, 2, MPI_INT, MPI_ANY_SOURCE,
                  MPIDYNRES_TAG_PSET_LOOKUP,
                  scheduler->config->base_communicator,
                  &requests[pset_lookup_request]);
        break;
      }
      case pset_op_request: {
        MPIDYNRES_scheduler_handle_pset_op(scheduler, &status, &pset_op_msg);

        // create new request
        MPI_Irecv(&pset_op_msg, 1, get_pset_op_datatype(), MPI_ANY_SOURCE,
                  MPIDYNRES_TAG_PSET_OP, scheduler->config->base_communicator,
                  &requests[pset_op_request]);
        break;
      }
      case pset_free_request: {
        MPIDYNRES_scheduler_handle_pset_free(scheduler, &status,
                                             &pset_free_msg);

        // create new request
        MPI_Irecv(&pset_free_msg, 1, get_pset_op_datatype(), MPI_ANY_SOURCE,
                  MPIDYNRES_TAG_PSET_OP, scheduler->config->base_communicator,
                  &requests[pset_op_request]);
        break;
      }

      case rc_request: {
        MPIDYNRES_scheduler_handle_rc(scheduler, &status);

        // create new request
        MPI_Irecv(&unused, 1, MPI_INT, MPI_ANY_SOURCE, MPIDYNRES_TAG_RC,
                  scheduler->config->base_communicator, &requests[rc_request]);
        break;
      }
      case rc_accept_request: {
        MPIDYNRES_scheduler_handle_rc_accept(scheduler, &status,
                                             &rc_accept_msg);

        // create new request
        MPI_Irecv(&rc_accept_msg, 1, get_rc_datatype(), MPI_ANY_SOURCE,
                  MPIDYNRES_TAG_RC_ACCEPT, scheduler->config->base_communicator,
                  &requests[rc_accept_request]);
        break;
      }
      default: {
        die("Request not implemented: %d\n", index);
        break;
      }
    };
  }
}




/*
 * PUBLIC METHODS
 */

/**
 * @brief      constructor for a new MPIDYNRES_scheduler
 *
 * @param      i_config the config to be used
 *
 * @return     the newly created scheduler object
 */
MPIDYNRES_scheduler *MPIDYNRES_scheduler_create(MPIDYNRESSIM_config *i_config) {
  int size;
  MPIDYNRES_scheduler *result = calloc(1, sizeof(MPIDYNRES_scheduler));
  if (result == NULL) {
    die("Memory Error!\n");
  };

  MPI_Comm_size(i_config->base_communicator, &size);
  if (size < 2) {
    die("Cannot schedule on a communicator which contains less than 2 "
        "ranks\n");
  }

  result->num_scheduling_processes = size - 1;

  result->config = i_config;

  result->next_session_id = 0;
  result->next_rc_tag = 0;
  result->pending_resource_change = false;

  result->running_crs = set_int_init(int_compare);
  result->pending_shutdowns = set_int_init(int_compare);
  result->pset_name_map = set_pset_node_init(pset_node_compare);
  result->rc_map = set_rc_node_init(rc_node_compare);
  result->process_states = set_process_state_init(process_state_compare);

  return result;
}

/**
 * @brief      destructor for MPIDYNRES_scheduler
 *
 * @param      scheduler the scheduler
 */
void MPIDYNRES_scheduler_destroy(MPIDYNRES_scheduler *scheduler) {
  set_int_free(&scheduler->running_crs);
  set_int_free(&scheduler->pending_shutdowns);
  set_pset_node_free(&scheduler->pset_name_map);
  set_rc_node_free(&scheduler->rc_map);
  set_process_state_free(&scheduler->process_states);

  free(scheduler);
}

/**
 * @brief      send the start signal to the initial number of crs
 *
 * @param      scheduler the scheduler
 */
void MPIDYNRES_start_first_crs(MPIDYNRES_scheduler *scheduler) {
  set_int initial_pset = set_int_init(int_compare);

  for (size_t i = 1; i <= scheduler->config->num_init_crs; i++) {
    set_int_insert(&scheduler->running_crs, i);
    set_int_insert(&initial_pset, i);
  }

  // create initial pset
  pset_node initial_pset_node = {
      .pset_name = "mpidynres://INIT",
      .pset_info = MPI_INFO_NULL,
      .pset = initial_pset,
  };
  char const * const initial_info_vec[] = {
    "mpidynres_initial", "yes",
    // TODO: what else could get in here?
  };
  int err = MPIDYNRES_Info_create_strings(COUNT_OF(initial_info_vec), initial_info_vec, &initial_pset_node.pset_info);
  if (err) {
    die("Failed to create mpi info when creating initial pset info\n");
  }

  set_pset_node_insert(&scheduler->pset_name_map, initial_pset_node);

  /*// the above will copy the pset*/
  /*set_int_free(&initial_pset);*/
  /*// the above will copy the info object*/
  /*MPI_Info_free(&initial_pset_node.pset_info);*/

  // actually start the psets
  for (size_t i = 1; i <= scheduler->config->num_init_crs; i++) {
    debug("Starting rank %zu with uri %s\n", i, initial_pset_node.pset_name);
    MPIDYNRES_scheduler_start_cr(scheduler, i, false, -1);
  }
}


/**
 * @brief      start first crs then start scheduling
 *
 * @param      scheduler the scheduler
 */
void MPIDYNRES_scheduler_start(MPIDYNRES_scheduler *scheduler) {
  debug("Starting scheduler...\n");
  init_log(scheduler);

  MPIDYNRES_start_first_crs(scheduler);

  // run until no more simulated processes running
  MPIDYNRES_scheduler_schedule(scheduler);

  // shutdown crs
  debug("No more running crs. Shutting down everything\n");
  MPIDYNRES_scheduler_shutdown_all_crs(scheduler);
}
