#include "scheduler.h"
// TODO: Improve error handling

#include <mpi.h>
#include <stdlib.h>
#include <string.h>

#include "datastructures/mpidynres_pset.h"
#include "datastructures/rc_table.h"
#include "datastructures/uri_table.h"

#include "comm.h"
#include "logging.h"
#include "mpidynres.h"
#include "scheduling_modes.h"
#include "util.h"
#include "scheduler_handlers.h"


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
void MPIDYNRES_scheduler_start_cr(MPIDYNRES_scheduler *scheduler,
                                         int i_cr) {
  MPIDYNRES_idle_command command = {
      .command_type = start,
  };

  MPI_Send(&command, 1, get_idle_command_datatype(), i_cr,
           MPIDYNRES_TAG_IDLE_COMMAND, scheduler->config->base_communicator);
  set_state(i_cr, running);
  log_state("Starting cr id %d", i_cr);
}

/**
 * @brief      Send shutdown signal to an idleing computing resource
 *
 * @details    Send a shutdown signal to the cr with id i_cr, it will then exit
 * the simulation and return
 *
 * @param      scheduler the scheduler
 *
 * @param      i_cr the cr_id of the cr to send the signal to
 */
void MPIDYNRES_scheduler_shutdown_cr(MPIDYNRES_scheduler *scheduler,
                                            int i_cr) {
  MPIDYNRES_idle_command command = {
      .command_type = shutdown,
  };
  assert(!MPIDYNRES_pset_contains(scheduler->running_crs, i_cr));

  if (scheduler->infos[i_cr] != MPI_INFO_NULL) {
    MPI_Info_free(&scheduler->infos[i_cr]);
  }

  MPI_Send(&command, 1, get_idle_command_datatype(), i_cr,
           MPIDYNRES_TAG_IDLE_COMMAND, scheduler->config->base_communicator);
}

/**
 * @brief      Send shutdown signal to all computing resources
 *
 * @param      scheduler the scheduler
 */
void MPIDYNRES_scheduler_shutdown_all_crs(
    MPIDYNRES_scheduler *scheduler) {
  assert(scheduler->running_crs->size == 0);
  // TODO: is there a send to all function?
  for (int cr = 1; cr < 1 + scheduler->num_crs; cr++) {
    MPIDYNRES_scheduler_shutdown_cr(scheduler, cr);
  }
}


/**
 * @brief      the main loop of the scheduler
 *
 * @details    the most important function of the scheduler, it is waiting for
 * different requests, starts the handler and gets back to waiting. when all crs
 * are idle, it will shut them all down and return
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

  while (scheduler->running_crs->size > 0) {
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

  result->config = i_config;

  result->next_session_id = 0;

  MPI_Comm_size(result->config->base_communicator, &size);
  if (size < 2) {
    die("Cannot schedule on a communicator which contains less than 2 "
        "ranks\n");
  }
  result->running_crs =
      MPIDYNRES_pset_create(size);  // rank 0 should always stay empty!!!!!!
  if (result->running_crs == NULL) {
    die("Failed to create set of running crs\n");
  }
  result->uri_table = MPIDYNRES_uri_table_create();
  if (result->uri_table == NULL) {
    die("Failed to create uri lookup table\n");
  }
  result->rc_table = rc_table_create();
  if (result->rc_table == NULL) {
    die("Failed to create resource change lookup table\n");
  }
  result->pending_shutdowns = MPIDYNRES_pset_create(size);
  if (result->pending_shutdowns == NULL) {
    die("Failed to create set of for pending_shutdowns crs\n");
  }
  result->infos = calloc(sizeof(MPI_Info), size);
  for (size_t i = 0; i < (size_t)size; i++) {
    result->infos[i] = MPI_INFO_NULL;
  }

  result->num_crs = size - 1;
  return result;
}

/**
 * @brief      destructor for MPIDYNRES_scheduler
 *
 * @param      scheduler the scheduler
 */
void MPIDYNRES_scheduler_destroy(MPIDYNRES_scheduler *scheduler) {
  MPIDYNRES_pset_destroy(scheduler->running_crs);
  MPIDYNRES_pset_destroy(scheduler->pending_shutdowns);
  MPIDYNRES_uri_table_destroy(scheduler->uri_table);
  rc_table_destroy(scheduler->rc_table);
  for (size_t i = 0; i < (size_t)scheduler->num_crs + 1; i++) {
    if (scheduler->infos[i] != MPI_INFO_NULL) {
      MPI_Info_free(&scheduler->infos[i]);
    }
  }
  free(scheduler->infos);
  free(scheduler);
}

/**
 * @brief      send the start signal to the inital number of crs
 *
 * @param      scheduler the scheduler
 */
void MPIDYNRES_start_first_crs(MPIDYNRES_scheduler *scheduler) {
  char init_uri[MPI_MAX_PSET_NAME_LEN];
  // start rank 1..num_init_crs
  for (size_t i = 1; i <= scheduler->config->num_init_crs; i++) {
    MPIDYNRES_pset_add_cr(&scheduler->running_crs, i);
  }

  // create uri for inital running set
  MPIDYNRES_uri_table_add_pset(scheduler->uri_table,
                               MPIDYNRES_pset_from_set(scheduler->running_crs),
                               MPI_INFO_NULL, init_uri);

  for (size_t i = 1; i <= scheduler->config->num_init_crs; i++) {
    debug("Starting rank %zu with uri %s\n", i, init_uri);
    MPIDYNRES_scheduler_start_cr(scheduler, i);
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
