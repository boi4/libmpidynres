#include "scheduler.h"
// TODO: Improve error handling
// TODO: add some smart uri garbage collection

#include <mpi.h>
#include <stdlib.h>
#include <string.h>

#include "comm.h"
#include "datastructures/mpidynres_pset_private.h"
#include "datastructures/init_info_table.h"
#include "datastructures/rc_table.h"
#include "datastructures/uri_table.h"
#include "mpidynres.h"
#include "mpidynres_pset.h"
#include "logging.h"
#include "scheduling_modes.h"
#include "util.h"

/*
 * PRIVATE METHODS
 */
/**
 * @brief      send the "start" command to a cr
 * TODO: add assertions that starting things are not running already
 *
 * @param      scheduler the scheduler
 *
 * @param      i_cr the cr id of the cr to start
 *
 * @return     return type
 */
static void MPIDYNRES_scheduler_start_cr(MPIDYNRES_scheduler *scheduler, int i_cr) {
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
static void MPIDYNRES_scheduler_shutdown_cr(MPIDYNRES_scheduler *scheduler,
                                         int i_cr) {
  MPIDYNRES_idle_command command = {
      .command_type = shutdown,
  };

  assert(!MPIDYNRES_pset_contains(scheduler->running_crs, i_cr));

  MPI_Send(&command, 1, get_idle_command_datatype(), i_cr,
           MPIDYNRES_TAG_IDLE_COMMAND, scheduler->config->base_communicator);
}

/**
 * @brief      Send shutdown signal to all computing resources
 *
 * @param      scheduler the scheduler
 */
static void MPIDYNRES_scheduler_shutdown_all_crs(MPIDYNRES_scheduler *scheduler) {
  assert(scheduler->running_crs->size == 0);
  // TODO: is there a send to all function?
  for (int cr = 1; cr < 1 + scheduler->num_crs; cr++) {
    MPIDYNRES_scheduler_shutdown_cr(scheduler, cr);
  }
}

/*
 * HANDLERS
 */

/**
 * @brief      handle a worker done message
 *
 * @details    remove the source rank (cr_id) from currently running and change
 * state to idle
 *
 * @param      scheduler the scheduler
 *
 * @param      status the MPI status of the message
 */
void MPIDYNRES_scheduler_handle_worker_done(MPIDYNRES_scheduler *scheduler,
                                         MPI_Status *status) {
  if (!MPIDYNRES_pset_contains(scheduler->running_crs, status->MPI_SOURCE)) {
    debug("ERROR: expected %d to not run, but got worker done msg\n",
          status->MPI_SOURCE);
  }
  debug("Rank %d has returned. Removing from running cr set and info table\n",
        status->MPI_SOURCE);

  // remove from pending shutdowns if in there
  if (MPIDYNRES_pset_contains(scheduler->pending_shutdowns, status->MPI_SOURCE)) {
    debug("Removing %d from pending shutdowns\n", status->MPI_SOURCE);
    MPIDYNRES_pset_remove_cr(scheduler->pending_shutdowns, status->MPI_SOURCE);
  }

  init_info_table_rem_entry(scheduler->info_table, status->MPI_SOURCE);

  /* printf("LIBMPIDYNRES : REMOVING %d\n\n\n", status->MPI_SOURCE); */
  /* MPIDYNRES_print_set(scheduler->running_crs); */
  /* printf("\n\n"); */
  MPIDYNRES_pset_remove_cr(scheduler->running_crs, status->MPI_SOURCE);

  set_state(status->MPI_SOURCE, idle);
  log_state("cr id %d returned/exited", status->MPI_SOURCE);
}

/**
 * @brief      handle a URI lookup message
 *
 * @details    look for the pset in the lookup table and send it to the
 * requesting create
 *
 * @param      scheduler the scheduler
 *
 * @param      status the MPI status of the message
 *
 * @param      uri the URI that should be looked up
 */
static void MPIDYNRES_scheduler_handle_uri_lookup(MPIDYNRES_scheduler *scheduler,
                                               MPI_Status *status,
                                               char uri[MPI_MAX_PSET_NAME_LEN]) {
  MPI_Datatype t;
  MPIDYNRES_pset *set;
  bool in_there;
  size_t cap;

  debug("Lookup request from rank %d about uri %s\n", status->MPI_SOURCE, uri);

  uri[MPI_MAX_PSET_NAME_LEN - 1] = '\0';
  in_there = MPIDYNRES_uri_table_lookup(scheduler->uri_table, uri, &set);
  /* debug("============"); */
  /* MPIDYNRES_print_set(set); */
  /* debug("============"); */

  cap = in_there ? set->_cap : MPIDYNRES_CR_SET_INVALID;

  MPI_Send(&cap, 1, my_MPI_SIZE_T, status->MPI_SOURCE,
           MPIDYNRES_TAG_URI_LOOKUP_ANSWER_CAP,
           scheduler->config->base_communicator);

  // only send if url valid
  if (in_there) {
    t = get_pset_datatype(cap);
    MPI_Send(set, 1, t, status->MPI_SOURCE, MPIDYNRES_TAG_URI_LOOKUP_ANSWER,
             scheduler->config->base_communicator);
    MPI_Type_free(&t);
  }
}

/**
 * @brief      handle a resource change message
 *
 * @details    depending on the config, call different scheduleing mode
 * functions defined in scheduling_modes.c
 *
 * @param      scheduler the scheduler
 *
 * @param      status the MPI status of the message
 */
static void MPIDYNRES_scheduler_handle_rc(MPIDYNRES_scheduler *scheduler,
                                       MPI_Status *status) {
  MPIDYNRESSIM_config *c = scheduler->config;
  MPIDYNRES_RC_msg rc_msg;

  // if pending, shutdowns, return none
  if (scheduler->pending_shutdowns->size > 0) {
    debug("There are still pending shutdowns\n");
    rc_msg.type = MPIDYNRES_RC_NONE;
  } else {
    switch (c->scheduling_mode) {
      case MPIDYNRES_MODE_RANDOM_DIFF: {
        random_diff_scheduling(scheduler, &rc_msg);
        break;
      }
    case MPIDYNRES_MODE_INC: {
        inc_scheduling(scheduler, &rc_msg);
        break;
      }
    case MPIDYNRES_MODE_INC_DEC: {
        inc_dec_scheduling(scheduler, &rc_msg);
        break;
      }
    }
  }

  // add rc to lookup table
  if (rc_msg.type != MPIDYNRES_RC_NONE) {
    rc_msg.tag = rc_table_add_entry(scheduler->rc_table, rc_msg);
  } else {
    rc_msg.tag = -1;
  }

  if (rc_msg.type == MPIDYNRES_RC_SUB) {
    // add to pending shutdowns
    MPIDYNRES_pset *set;
    MPIDYNRES_uri_table_lookup(scheduler->uri_table, rc_msg.uri, &set);
    for (size_t i = 0; i < set->size; i++) {
      if (MPIDYNRES_pset_contains(scheduler->running_crs, set->cr_ids[i])) {
        MPIDYNRES_pset_add_cr(&scheduler->pending_shutdowns, set->cr_ids[i]);
      }
    }
  }
  // send anwer
  MPI_Send(&rc_msg, 1, get_rc_datatype(), status->MPI_SOURCE,
           MPIDYNRES_TAG_RC_ANSWER, scheduler->config->base_communicator);
}

/**
 * @brief      handle a resource change accept message
 *
 * @details    depending on the rc type, start new crs or add crs to pending
 * shutdowns
 *
 * @param      scheduler the scheduler
 *
 * @param      status the MPI status of the message
 *
 * @param      rc_accept_msg the resource change accept message struct with more
 * information about the rc
 */
static void MPIDYNRES_scheduler_handle_rc_accept(
    MPIDYNRES_scheduler *scheduler, MPI_Status *status,
    MPIDYNRES_RC_accept_msg *rc_accept_msg) {
  MPIDYNRES_pset *set;
  MPIDYNRES_init_info init_info;

  (void)status;

  MPIDYNRES_RC_msg rc_msg =
      rc_table_get_entry(scheduler->rc_table, rc_accept_msg->rc_tag);
  if (rc_msg.tag == -1) {
    die("Inavalid rc_accept tag: %d, was already accepted or never created\n",
        rc_msg.tag);
  }
  rc_table_rem_entry(scheduler->rc_table, rc_accept_msg->rc_tag);

  if (rc_msg.type == MPIDYNRES_RC_ADD || rc_msg.type == MPIDYNRES_RC_REPLACE) {
    MPIDYNRES_uri_table_lookup(scheduler->uri_table, rc_msg.uri, &set);

    init_info.init_tag = rc_accept_msg->new_process_tag;
    strncpy(init_info.uri_init, rc_msg.uri, MPI_MAX_PSET_NAME_LEN - 1);
    strncpy(init_info.uri_passed, rc_accept_msg->uri, MPI_MAX_PSET_NAME_LEN - 1);

    // insert init infos and start workers
    for (size_t i = 0; i < set->size; i++) {
      debug("Starting rank %d with passed uri %s init uri %s, init tag %d\n",
            set->cr_ids[i], init_info.uri_passed, init_info.uri_init,
            init_info.init_tag);
      init_info.cr_id = set->cr_ids[i];
      init_info_table_add_entry(scheduler->info_table, init_info);

      MPIDYNRES_scheduler_start_cr(scheduler, set->cr_ids[i]);
    }

    /* MPIDYNRES_pset_destroy(set); */
  } else if (rc_msg.type == MPIDYNRES_RC_SUB) {
    MPIDYNRES_uri_table_lookup(scheduler->uri_table, rc_msg.uri, &set);
    for (size_t i = 0; i < set->size; i++) {
      if (MPIDYNRES_pset_contains(scheduler->running_crs, set->cr_ids[i])) {
        set_state(set->cr_ids[i], accepted_shutdown);
      }
    }
    log_state("accepted shutdown of crs");
  }
}

/**
 * @brief      handle an uri op message
 *
 * @details    create a new uri from multiple uris and a set operation, send the
 * new uri back to the cr
 *
 * @param      scheduler the scheduler
 *
 * @param      status the MPI status of the message
 *
 * @param      uri_op_msg the URI operation message content containing the uris
 * and the operation itself
 */
static void MPIDYNRES_scheduler_handle_uri_op(MPIDYNRES_scheduler *scheduler,
                                           MPI_Status *status,
                                           MPIDYNRES_URI_op_msg *uri_op_msg) {
  char res_uri[MPI_MAX_PSET_NAME_LEN] = {0};
  MPIDYNRES_pset *set1;
  MPIDYNRES_pset *set2;
  MPIDYNRES_pset *res_set;
  bool ok;

  ok = MPIDYNRES_uri_table_lookup(scheduler->uri_table, uri_op_msg->uri1, &set1);
  if (!ok) {
    res_uri[0] = '\0';
    MPI_Send(res_uri, MPI_MAX_PSET_NAME_LEN, MPI_CHAR, status->MPI_SOURCE,
             MPIDYNRES_TAG_URI_OP_ANSWER, scheduler->config->base_communicator);
    return;
  }
  ok = MPIDYNRES_uri_table_lookup(scheduler->uri_table, uri_op_msg->uri2, &set2);
  if (!ok) {
    res_uri[0] = '\0';
    MPI_Send(res_uri, MPI_MAX_PSET_NAME_LEN, MPI_CHAR, status->MPI_SOURCE,
             MPIDYNRES_TAG_URI_OP_ANSWER, scheduler->config->base_communicator);
    return;
  }
  res_set = MPIDYNRES_pset_from_set(set1);
  switch (uri_op_msg->op) {
    case MPIDYNRES_URI_UNION: {
      MPIDYNRES_pset_union(&res_set, set2);
      break;
    }
    case MPIDYNRES_URI_INTERSECT: {
      MPIDYNRES_pset_intersect(res_set, set2);
      break;
    }
    case MPIDYNRES_URI_SUBTRACT: {
      MPIDYNRES_pset_subtract(res_set, set2);
      break;
    }
  }
  MPIDYNRES_uri_table_add_pset(scheduler->uri_table, res_set, res_uri);
  MPI_Send(res_uri, MPI_MAX_PSET_NAME_LEN, MPI_CHAR, status->MPI_SOURCE,
           MPIDYNRES_TAG_URI_OP_ANSWER, scheduler->config->base_communicator);
}

/**
 * @brief      handle init info messages
 *
 * @details    lookup the init info of the requesting cr and send the answer
 * back
 *
 * @param      scheduler the scheduler
 *
 * @param      status the MPI status of the message
 */
static void MPIDYNRES_scheduler_handle_init_info(MPIDYNRES_scheduler *scheduler,
                                              MPI_Status *status) {
  int cr_id = status->MPI_SOURCE;
  MPIDYNRES_init_info info =
      init_info_table_get_entry(scheduler->info_table, cr_id);
  if (info.cr_id != cr_id) {
    die("Couldn't get init info for %d\n", cr_id);
  }
  MPI_Send(&info, 1, get_init_info_datatype(), status->MPI_SOURCE,
           MPIDYNRES_TAG_INIT_INFO_ANSWER, scheduler->config->base_communicator);
}

/**
 * @brief      handle uri size lookup message
 *
 * @details    check the size of the cr set in the lookup table and send it as
 * the answer
 *
 * @param      scheduler the scheduler
 *
 * @param      status the MPI status of the message
 *
 * @param      uri the uri that the size was requested of
 */
static void MPIDYNRES_scheduler_handle_uri_size(MPIDYNRES_scheduler *scheduler,
                                             MPI_Status *status,
                                             char uri[MPI_MAX_PSET_NAME_LEN]) {
  MPIDYNRES_pset *set;
  MPIDYNRES_uri_table_lookup(scheduler->uri_table, uri,
                          &set);  // TODO: handle error
  MPI_Send(&set->size, 1, my_MPI_SIZE_T, status->MPI_SOURCE,
           MPIDYNRES_TAG_URI_SIZE_ANSWER, scheduler->config->base_communicator);
}

/**
 * @brief      handle uri free message
 *
 * @details    remove uri from lookup table
 *
 * @param      scheduler the scheduler
 *
 * @param      status the MPI status of the message
 *
 * @param      uri the uri that should be freed
 */
static void MPIDYNRES_scheduler_handle_uri_free(MPIDYNRES_scheduler *scheduler,
                                             MPI_Status *status,
                                             char uri[MPI_MAX_PSET_NAME_LEN]) {
  (void)status;
  MPIDYNRES_uri_table_uri_free(scheduler->uri_table, uri);
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
  int unused;
  char uri[MPI_MAX_PSET_NAME_LEN] = {0};
  MPIDYNRES_URI_op_msg uri_op_msg = {0};
  MPIDYNRES_RC_accept_msg rc_accept_msg;

  enum {
    worker_done_request,
    uri_lookup_request,
    uri_op_request,
    init_info_request,
    uri_size_request,
    uri_free_request,

    rc_request,
    rc_accept_request,

    REQUEST_COUNT
  };

  MPI_Request requests[REQUEST_COUNT];

  MPI_Irecv(&unused, 1, MPI_INT, MPI_ANY_SOURCE, MPIDYNRES_TAG_DONE_RUNNING,
            scheduler->config->base_communicator,
            &requests[worker_done_request]);
  MPI_Irecv(uri, MPI_MAX_PSET_NAME_LEN, MPI_CHAR, MPI_ANY_SOURCE,
            MPIDYNRES_TAG_URI_LOOKUP, scheduler->config->base_communicator,
            &requests[uri_lookup_request]);
  MPI_Irecv(&uri_op_msg, 1, get_uri_op_datatype(), MPI_ANY_SOURCE,
            MPIDYNRES_TAG_URI_OP, scheduler->config->base_communicator,
            &requests[uri_op_request]);
  MPI_Irecv(uri, MPI_MAX_PSET_NAME_LEN, MPI_CHAR, MPI_ANY_SOURCE,
            MPIDYNRES_TAG_URI_FREE, scheduler->config->base_communicator,
            &requests[uri_free_request]);
  MPI_Irecv(&unused, 1, MPI_INT, MPI_ANY_SOURCE, MPIDYNRES_TAG_INIT_INFO,
            scheduler->config->base_communicator, &requests[init_info_request]);
  MPI_Irecv(uri, MPI_MAX_PSET_NAME_LEN, MPI_CHAR, MPI_ANY_SOURCE,
            MPIDYNRES_TAG_URI_SIZE, scheduler->config->base_communicator,
            &requests[uri_size_request]);
  MPI_Irecv(&rc_accept_msg, 1, get_rc_accept_datatype(), MPI_ANY_SOURCE,
            MPIDYNRES_TAG_RC_ACCEPT, scheduler->config->base_communicator,
            &requests[rc_accept_request]);
  MPI_Irecv(&unused, 1, MPI_INT, MPI_ANY_SOURCE, MPIDYNRES_TAG_RC,
            scheduler->config->base_communicator, &requests[rc_request]);

  // TODO: what happens if messages arrive in wrong oder and for short time it
  // looks like set is empty?
  while (scheduler->running_crs->size > 0) {
    debug("Waiting for commands...\n");
    MPI_Waitany(REQUEST_COUNT, requests, &index, &status);
    switch (index) {
      case worker_done_request: {
        MPIDYNRES_scheduler_handle_worker_done(scheduler, &status);

        // create new request
        MPI_Irecv(&unused, 1, MPI_INT, MPI_ANY_SOURCE, MPIDYNRES_TAG_DONE_RUNNING,
                  scheduler->config->base_communicator,
                  &requests[worker_done_request]);
        break;
      }
      case uri_lookup_request: {
        MPIDYNRES_scheduler_handle_uri_lookup(scheduler, &status, uri);

        // create new request
        MPI_Irecv(uri, MPI_MAX_PSET_NAME_LEN, MPI_CHAR, MPI_ANY_SOURCE,
                  MPIDYNRES_TAG_URI_LOOKUP, scheduler->config->base_communicator,
                  &requests[uri_lookup_request]);
        break;
      }
      case uri_op_request: {
        MPIDYNRES_scheduler_handle_uri_op(scheduler, &status, &uri_op_msg);

        // create new request
        MPI_Irecv(&uri_op_msg, 1, get_uri_op_datatype(), MPI_ANY_SOURCE,
                  MPIDYNRES_TAG_URI_OP, scheduler->config->base_communicator,
                  &requests[uri_op_request]);
        break;
      }
      case rc_request: {
        MPIDYNRES_scheduler_handle_rc(scheduler, &status);

        // create new request
        MPI_Irecv(&unused, 1, MPI_INT, MPI_ANY_SOURCE, MPIDYNRES_TAG_RC,
                  scheduler->config->base_communicator, &requests[rc_request]);
        break;
      }
      case init_info_request: {
        MPIDYNRES_scheduler_handle_init_info(scheduler, &status);

        // create new request
        MPI_Irecv(&unused, 1, MPI_INT, MPI_ANY_SOURCE, MPIDYNRES_TAG_INIT_INFO,
                  scheduler->config->base_communicator,
                  &requests[init_info_request]);
        break;
      }
      case uri_size_request: {
        MPIDYNRES_scheduler_handle_uri_size(scheduler, &status, uri);

        // create new request
        MPI_Irecv(uri, MPI_MAX_PSET_NAME_LEN, MPI_CHAR, MPI_ANY_SOURCE,
                  MPIDYNRES_TAG_URI_SIZE, scheduler->config->base_communicator,
                  &requests[uri_size_request]);
        break;
      }
      case rc_accept_request: {
        MPIDYNRES_scheduler_handle_rc_accept(scheduler, &status, &rc_accept_msg);

        // create new request
        MPI_Irecv(&rc_accept_msg, 1, get_rc_accept_datatype(), MPI_ANY_SOURCE,
                  MPIDYNRES_TAG_RC_ACCEPT, scheduler->config->base_communicator,
                  &requests[rc_accept_request]);
        break;
      }
      case uri_free_request: {
        MPIDYNRES_scheduler_handle_uri_free(scheduler, &status, uri);

        // create new request
        MPI_Irecv(uri, MPI_MAX_PSET_NAME_LEN, MPI_CHAR, MPI_ANY_SOURCE,
                  MPIDYNRES_TAG_URI_FREE, scheduler->config->base_communicator,
                  &requests[uri_free_request]);
        break;
      }
      default: {
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
  result->info_table = init_info_table_create();
  if (result->info_table == NULL) {
    die("Failed to create init info lookup table\n");
  }
  result->rc_table = rc_table_create();
  if (result->rc_table == NULL) {
    die("Failed to create resource change lookup table\n");
  }
  result->pending_shutdowns = MPIDYNRES_pset_create(size);
  if (result->pending_shutdowns == NULL) {
    die("Failed to create set of for pending_shutdowns crs\n");
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
  /* MPIDYNRES_pset_destroy(scheduler->running_crs); */
  /* MPIDYNRES_pset_destroy(scheduler->pending_shutdowns); */
  MPIDYNRES_uri_table_destroy(scheduler->uri_table);
  init_info_table_destroy(scheduler->info_table);
  rc_table_destroy(scheduler->rc_table);
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
                              init_uri);

  MPIDYNRES_init_info info = {0};
  info.init_tag = MPIDYNRES_TAG_FIRST_START;
  strncpy(info.uri_init, init_uri, MPI_MAX_PSET_NAME_LEN - 1);

  for (size_t i = 1; i <= scheduler->config->num_init_crs; i++) {
    debug("Starting rank %zu with uri %s\n", i, init_uri);
    info.cr_id = i;
    init_info_table_add_entry(scheduler->info_table, info);
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
