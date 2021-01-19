#include "scheduler.h"
// TODO: Improve error handling
// TODO: add some smart uri garbage collection

#include <mpi.h>
#include <stdlib.h>
#include <string.h>

#include "comm.h"
#include "datastructures/mpidynres_pset.h"
#include "datastructures/rc_table.h"
#include "datastructures/uri_table.h"
#include "logging.h"
#include "mpidynres.h"
#include "mpidynres_pset.h"
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
static void MPIDYNRES_scheduler_start_cr(MPIDYNRES_scheduler *scheduler,
                                         int i_cr) {
  MPIDYNRES_idle_command command = {
      .command_type = start,
  };
  assert(!MPIDYNRES_pset_contains(scheduler->running_crs, i_cr));

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
static void MPIDYNRES_scheduler_shutdown_all_crs(
    MPIDYNRES_scheduler *scheduler) {
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

  // remove from pending shutdowns if in there
  if (MPIDYNRES_pset_contains(scheduler->pending_shutdowns,
                              status->MPI_SOURCE)) {
    debug("Removing %d from pending shutdowns\n", status->MPI_SOURCE);
    MPIDYNRES_pset_remove_cr(scheduler->pending_shutdowns, status->MPI_SOURCE);
  }

  /* printf("LIBMPIDYNRES : REMOVING %d\n\n\n", status->MPI_SOURCE); */
  /* MPIDYNRES_print_set(scheduler->running_crs); */
  /* printf("\n\n"); */
  MPIDYNRES_pset_remove_cr(scheduler->running_crs, status->MPI_SOURCE);

  set_state(status->MPI_SOURCE, idle);
  log_state("cr id %d returned/exited", status->MPI_SOURCE);
}

// MPIDYNRES_scheduler_handle_session_create(scheduler, &status);

/**
 * @brief      handle a pset lookup message
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
static void MPIDYNRES_scheduler_handle_pset_lookup(
    MPIDYNRES_scheduler *scheduler, MPI_Status *status,
    MPIDYNRES_pset_free_msg *pset_free_msg) {
  MPI_Datatype t;
  MPIDYNRES_pset *set;
  MPI_Info info;
  bool in_there;
  size_t cap;
  char *uri = pset_free_msg->pset_name;

  debug("Lookup request from rank %d about uri %s\n", status->MPI_SOURCE, uri);

  uri[MPI_MAX_PSET_NAME_LEN - 1] = '\0';
  in_there = MPIDYNRES_uri_table_lookup(scheduler->uri_table, uri, &set, &info);
  /* debug("============"); */
  /* MPIDYNRES_print_set(set); */
  /* debug("============"); */

  cap = in_there ? set->_cap : MPIDYNRES_CR_SET_INVALID;

  MPI_Send(&cap, 1, my_MPI_SIZE_T, status->MPI_SOURCE,
           MPIDYNRES_TAG_PSET_LOOKUP_ANSWER_CAP,
           scheduler->config->base_communicator);

  // only send if url valid
  if (in_there) {
    t = get_pset_datatype(cap);
    MPI_Send(set, 1, t, status->MPI_SOURCE, MPIDYNRES_TAG_PSET_LOOKUP_ANSWER,
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
  MPI_Info info;

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
    MPIDYNRES_uri_table_lookup(scheduler->uri_table, rc_msg.uri, &set, &info);
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
  MPI_Info info;

  (void)status;

  MPIDYNRES_RC_msg rc_msg =
      rc_table_get_entry(scheduler->rc_table, rc_accept_msg->rc_tag);
  if (rc_msg.tag == -1) {
    die("Inavalid rc_accept tag: %d, was already accepted or never created\n",
        rc_msg.tag);
  }
  rc_table_rem_entry(scheduler->rc_table, rc_accept_msg->rc_tag);

  if (rc_msg.type == MPIDYNRES_RC_ADD || rc_msg.type == MPIDYNRES_RC_REPLACE) {
    MPIDYNRES_uri_table_lookup(scheduler->uri_table, rc_msg.uri, &set, &info);

    // insert init infos and start workers
    for (size_t i = 0; i < set->size; i++) {
      /*debug("Starting rank %d with passed uri %s init uri %s, init tag
       * %d\n",*/
      /*set->cr_ids[i], init_info.uri_passed, init_info.uri_init,*/
      /*init_info.init_tag);*/

      MPIDYNRES_scheduler_start_cr(scheduler, set->cr_ids[i]);
    }

    /* MPIDYNRES_pset_destroy(set); */
  } else if (rc_msg.type == MPIDYNRES_RC_SUB) {
    MPIDYNRES_uri_table_lookup(scheduler->uri_table, rc_msg.uri, &set, &info);
    for (size_t i = 0; i < set->size; i++) {
      if (MPIDYNRES_pset_contains(scheduler->running_crs, set->cr_ids[i])) {
        set_state(set->cr_ids[i], accepted_shutdown);
      }
    }
    log_state("accepted shutdown of crs");
  }
}

/**
 * @brief      handle a pset op message
 *
 * @details    create a new uri from multiple uris and a set operation, send the
 * new uri back to the cr
 *
 * @param      scheduler the scheduler
 *
 * @param      status the MPI status of the message
 *
 * @param      pset_op_msg the URI operation message content containing the uris
 * and the operation itself
 */
static void MPIDYNRES_scheduler_handle_pset_op(
    MPIDYNRES_scheduler *scheduler, MPI_Status *status,
    MPIDYNRES_pset_op_msg *pset_op_msg) {
  char res_uri[MPI_MAX_PSET_NAME_LEN] = {0};
  MPIDYNRES_pset *set1;
  MPIDYNRES_pset *set2;
  MPIDYNRES_pset *res_set;
  MPI_Info info;
  bool ok;

  ok = MPIDYNRES_uri_table_lookup(scheduler->uri_table, pset_op_msg->uri1,
                                  &set1, &info);
  if (!ok) {
    res_uri[0] = '\0';
    MPI_Send(res_uri, MPI_MAX_PSET_NAME_LEN, MPI_CHAR, status->MPI_SOURCE,
             MPIDYNRES_TAG_PSET_OP_ANSWER,
             scheduler->config->base_communicator);
    return;
  }
  ok = MPIDYNRES_uri_table_lookup(scheduler->uri_table, pset_op_msg->uri2,
                                  &set2, &info);
  if (!ok) {
    res_uri[0] = '\0';
    MPI_Send(res_uri, MPI_MAX_PSET_NAME_LEN, MPI_CHAR, status->MPI_SOURCE,
             MPIDYNRES_TAG_PSET_OP_ANSWER,
             scheduler->config->base_communicator);
    return;
  }
  res_set = MPIDYNRES_pset_from_set(set1);
  switch (pset_op_msg->op) {
    case MPIDYNRES_PSET_UNION: {
      MPIDYNRES_pset_union(&res_set, set2);
      break;
    }
    case MPIDYNRES_PSET_INTERSECT: {
      MPIDYNRES_pset_intersect(res_set, set2);
      break;
    }
    case MPIDYNRES_PSET_SUBTRACT: {
      MPIDYNRES_pset_subtract(res_set, set2);
      break;
    }
  }
  MPIDYNRES_uri_table_add_pset(scheduler->uri_table, res_set, MPI_INFO_NULL, res_uri);
  MPI_Send(res_uri, MPI_MAX_PSET_NAME_LEN, MPI_CHAR, status->MPI_SOURCE,
           MPIDYNRES_TAG_PSET_OP_ANSWER, scheduler->config->base_communicator);
}

static void MPIDYNRES_scheduler_handle_session_create(
    MPIDYNRES_scheduler *scheduler, MPI_Status *status) {
  int err;
  err = MPI_Send(&scheduler->next_session_id, 1, MPI_INT, status->MPI_SOURCE,
                 MPIDYNRES_TAG_SESSION_CREATE_ANSWER,
                 scheduler->config->base_communicator);
  if (err) {
    die("Error in MPI_Send\n");
  }
  scheduler->next_session_id++;
}

static void MPIDYNRES_scheduler_handle_session_finalize(
    MPIDYNRES_scheduler *scheduler, MPI_Status *status) {
  int err;
  int ok = 0;
  err = MPI_Send(&ok, 1, MPI_INT, status->MPI_SOURCE,
                 MPIDYNRES_TAG_SESSION_FINALIZE_ANSWER,
                 scheduler->config->base_communicator);
  if (err) {
    die("Error in MPI_Send\n");
  }
}

static void MPIDYNRES_scheduler_handle_num_psets(MPIDYNRES_scheduler *scheduler,
                                                 MPI_Status *status) {
  int err;
  int count = 0;
  MPI_Info info;  // unused for now

  err = MPIDYNRES_Recv_MPI_Info(&info, 0, MPIDYNRES_TAG_NUM_PSETS_INFO_SIZE,
                                MPIDYNRES_TAG_NUM_PSETS_INFO,
                                scheduler->config->base_communicator,
                                MPI_STATUS_IGNORE, MPI_STATUS_IGNORE);
  if (err) {
    die("Error in receiving mpi info\n");
  }

  for (size_t i = 0; i < scheduler->uri_table->num_entries; i++) {
    // TODO: add cache mechanism
    if (MPIDYNRES_pset_contains(scheduler->uri_table->mappings[i].set,
                                status->MPI_SOURCE)) {
      count += 1;
    }
  }
  err = MPI_Send(&count, 1, MPI_INT, status->MPI_SOURCE,
                 MPIDYNRES_TAG_NUM_PSETS_ANSWER,
                 scheduler->config->base_communicator);
  if (err) {
    die("Error in MPI send\n");
  }
}

static void MPIDYNRES_scheduler_handle_nth_psets(MPIDYNRES_scheduler *scheduler,
                                                 MPI_Status *status, int n,
                                                 int pset_max_len) {
  int err;
  int count = 0;
  size_t i = 0;
  MPI_Info info;  // unused for now

  assert(pset_max_len > 0);

  err = MPIDYNRES_Recv_MPI_Info(&info, 0, MPIDYNRES_TAG_NTH_PSET_INFO_SIZE,
                                MPIDYNRES_TAG_NTH_PSET_INFO,
                                scheduler->config->base_communicator,
                                MPI_STATUS_IGNORE, MPI_STATUS_IGNORE);
  if (err) {
    die("Error in receiving mpi info\n");
  }

  for (i = 0; count < n && i < scheduler->uri_table->num_entries; i++) {
    // TODO: add cache mechanism
    if (MPIDYNRES_pset_contains(scheduler->uri_table->mappings[i].set,
                                status->MPI_SOURCE)) {
      count += 1;
    }
  }
  if (i == scheduler->uri_table->num_entries) {
    // TODO error
  }
  int uri_size = strlen(scheduler->uri_table->mappings[i].uri) + 1;
  if (uri_size <= pset_max_len) {
    // fits
    err = MPI_Send(scheduler->uri_table->mappings[i].uri, uri_size, MPI_CHAR,
                   status->MPI_SOURCE, MPIDYNRES_TAG_NTH_PSET_ANSWER,
                   scheduler->config->base_communicator);
    if (err) {
      die("Error in MPI send\n");
    }
  } else {
    // doesn't fit
    char tmp = scheduler->uri_table->mappings[i].uri[pset_max_len];
    scheduler->uri_table->mappings[i].uri[pset_max_len] = '\0';
    err = MPI_Send(scheduler->uri_table->mappings[i].uri, pset_max_len,
                   MPI_CHAR, status->MPI_SOURCE, MPIDYNRES_TAG_NTH_PSET_ANSWER,
                   scheduler->config->base_communicator);
    scheduler->uri_table->mappings[i].uri[pset_max_len] = tmp;
    if (err) {
      die("Error in MPI send\n");
    }
  }
}

static void MPIDYNRES_scheduler_handle_pset_info(MPIDYNRES_scheduler *scheduler,
                                                 MPI_Status *status,
                                                 int strsize) {
  int err;
  MPIDYNRES_pset *set;
  MPI_Info info;

  assert(strsize > 0);
  char *name = calloc(sizeof(char), strsize);

  err = MPI_Recv(name, strsize, MPI_CHAR, status->MPI_SOURCE,
                 MPIDYNRES_TAG_PSET_INFO_NAME,
                 scheduler->config->base_communicator, MPI_STATUS_IGNORE);
  if (err) {
    die("Error in MPI recv\n");
  }
  name[strsize] = '\0';
  bool in_there =
      MPIDYNRES_uri_table_lookup(scheduler->uri_table, name, &set, &info);

  if (!in_there) {
    info = MPI_INFO_NULL;
  }
  err = MPIDYNRES_Send_MPI_Info(
      info, status->MPI_SOURCE, MPIDYNRES_TAG_PSET_INFO_ANSWER_SIZE,
      MPIDYNRES_TAG_PSET_INFO_ANSWER, scheduler->config->base_communicator);
  if (err) {
    die("Error in sending mpi info\n");
  }
}

static void MPIDYNRES_scheduler_handle_pset_free(
    MPIDYNRES_scheduler *scheduler, MPI_Status *status,
    char uri[MPI_MAX_PSET_NAME_LEN]) {
  (void) uri;
  (void) status;
  (void) scheduler;
  // TODO: mark as inactive
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
  int session_id;
  int msg[3];
  char uri[MPI_MAX_PSET_NAME_LEN] = {0};
  MPIDYNRES_RC_accept_msg rc_accept_msg = {0};
  MPIDYNRES_pset_op_msg pset_op_msg = {0};
  MPIDYNRES_pset_free_msg pset_free_msg = {0};

  enum {
    worker_done_request,

    session_create_request,
    session_finalize_request,

    num_psets_request,
    nth_pset_request,
    pset_info_request,

    pset_lookup_request,
    pset_op_request,
    pset_free_request,

    rc_request,
    rc_accept_request,

    REQUEST_COUNT
  };

  MPI_Request requests[REQUEST_COUNT];

  MPI_Irecv(&unused, 1, MPI_INT, MPI_ANY_SOURCE, MPIDYNRES_TAG_DONE_RUNNING,
            scheduler->config->base_communicator,
            &requests[worker_done_request]);

  MPI_Irecv(&unused, 1, MPI_INT, MPI_ANY_SOURCE, MPIDYNRES_TAG_SESSION_CREATE,
            scheduler->config->base_communicator,
            &requests[session_create_request]);
  MPI_Irecv(&session_id, 1, MPI_INT, MPI_ANY_SOURCE,
            MPIDYNRES_TAG_SESSION_FINALIZE,
            scheduler->config->base_communicator,
            &requests[session_finalize_request]);

  MPI_Irecv(&session_id, 1, MPI_INT, MPI_ANY_SOURCE, MPIDYNRES_TAG_NUM_PSETS,
            scheduler->config->base_communicator, &requests[num_psets_request]);
  MPI_Irecv(msg, 3, MPI_INT, MPI_ANY_SOURCE, MPIDYNRES_TAG_NTH_PSET,
            scheduler->config->base_communicator, &requests[nth_pset_request]);
  MPI_Irecv(msg, 2, MPI_INT, MPI_ANY_SOURCE, MPIDYNRES_TAG_PSET_INFO,
            scheduler->config->base_communicator, &requests[pset_info_request]);

  MPI_Irecv(msg, 2, MPI_INT, MPI_ANY_SOURCE, MPIDYNRES_TAG_PSET_LOOKUP,
            scheduler->config->base_communicator,
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
      case session_finalize_request: {
        MPIDYNRES_scheduler_handle_session_finalize(scheduler, &status);

        // create new request
        MPI_Irecv(&session_id, 1, MPI_INT, MPI_ANY_SOURCE,
                  MPIDYNRES_TAG_SESSION_FINALIZE,
                  scheduler->config->base_communicator,
                  &requests[session_finalize_request]);

        break;
      }

      case num_psets_request: {
        MPIDYNRES_scheduler_handle_num_psets(scheduler, &status);

        // create new request
        MPI_Irecv(&session_id, 1, MPI_INT, MPI_ANY_SOURCE,
                  MPIDYNRES_TAG_NUM_PSETS, scheduler->config->base_communicator,
                  &requests[num_psets_request]);
        break;
      }
      case nth_pset_request: {
        MPIDYNRES_scheduler_handle_nth_psets(scheduler, &status, msg[1],
                                             msg[2]);

        // create new request
        MPI_Irecv(msg, 3, MPI_INT, MPI_ANY_SOURCE, MPIDYNRES_TAG_NTH_PSET,
                  scheduler->config->base_communicator,
                  &requests[nth_pset_request]);
        break;
      }
      case pset_info_request: {
        MPIDYNRES_scheduler_handle_pset_info(scheduler, &status, msg[1]);

        // create new request
        MPI_Irecv(msg, 2, MPI_INT, MPI_ANY_SOURCE, MPIDYNRES_TAG_PSET_INFO,
                  scheduler->config->base_communicator,
                  &requests[pset_info_request]);
        break;
      }

      case pset_lookup_request: {
        MPIDYNRES_scheduler_handle_pset_lookup(scheduler, &status,
                                               &pset_free_msg);

        // create new request
        MPI_Irecv(&pset_free_msg, MPI_MAX_PSET_NAME_LEN, MPI_CHAR,
                  MPI_ANY_SOURCE, MPIDYNRES_TAG_PSET_LOOKUP,
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
        MPIDYNRES_scheduler_handle_pset_free(scheduler, &status, uri);

        // create new request
        MPI_Irecv(&pset_op_msg, 1, get_pset_op_datatype(), MPI_ANY_SOURCE,
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
