#include "logging.h"
#include "scheduler.h"
#include "scheduling_modes.h"
#include "string.h"
#include "util.h"

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
  int cr_id = status->MPI_SOURCE;
  if (set_int_count(&scheduler->running_crs, cr_id) != 1) {
    die("ERROR: expected %d to not run, but got worker done msg\n", cr_id);
  }

  // remove from pending shutdowns if in there
  if (set_int_count(&scheduler->pending_shutdowns, cr_id) == 1) {
    debug("Removing %d from pending shutdowns\n", cr_id);
    set_int_erase(&scheduler->pending_shutdowns, cr_id);
  }

  assert(set_process_state_count(&scheduler->process_states,
                                 (process_state){.process_id = cr_id}) == 1);
  // remove process state
  set_process_state_erase(&scheduler->process_states,
                          (process_state){.process_id = cr_id});

  // free all process sets containing the one?
  // TODO

  // remove from running processes
  set_int_erase(&scheduler->running_crs, cr_id);

  set_state(cr_id, idle);
  log_state("cr id %d returned/exited", cr_id);
}

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
 * @param      name_strsize the size of the name including the null byte
 */
void MPIDYNRES_scheduler_handle_pset_lookup(MPIDYNRES_scheduler *scheduler,
                                            MPI_Status *status,
                                            int name_strsize) {
  int err;
  int cr_id = status->MPI_SOURCE;
  size_t answer_size;
  debug("Lookup request from rank %d\n", cr_id);

  char *name = calloc(name_strsize, sizeof(char));
  if (name == NULL) {
    die("Memory Error\n");
  }

  err = MPI_Recv(name, name_strsize, MPI_CHAR, cr_id,
                 MPIDYNRES_TAG_PSET_LOOKUP_NAME,
                 scheduler->config->base_communicator, MPI_STATUS_IGNORE);
  if (err) {
    die("Failed to receive\n");
  }

  pset_node look_key = {0};
  strcpy(look_key.pset_name, name);
  set_pset_node_node *pnn =
      set_pset_node_find(&scheduler->pset_name_map, look_key);
  free(name);

  bool in_there = (pnn == NULL);
  answer_size = in_there ? pnn->key.pset.size : 0;

  MPI_Send(&answer_size, 1, my_MPI_SIZE_T, status->MPI_SOURCE,
           MPIDYNRES_TAG_PSET_LOOKUP_ANSWER_SIZE,
           scheduler->config->base_communicator);

  // only send if url valid
  if (in_there) {
    int *tmp = calloc(answer_size, sizeof(int));
    if (!tmp) {
      die("Memory Error\n");
    }
    int i = 0;
    foreach (set_int, &pnn->key.pset, it) { tmp[i++] = *it.ref; }
    MPI_Send(tmp, answer_size, MPI_INT, status->MPI_SOURCE,
             MPIDYNRES_TAG_PSET_LOOKUP_ANSWER,
             scheduler->config->base_communicator);
    free(tmp);
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
void MPIDYNRES_scheduler_handle_rc(MPIDYNRES_scheduler *scheduler,
                                   MPI_Status *status) {
  // TODO
  /*MPIDYNRESSIM_config *c = scheduler->config;*/
  /*MPIDYNRES_RC_msg rc_msg;*/
  /*MPI_Info info;*/

  /*// if pending, shutdowns, return none*/
  /*if (scheduler->pending_shutdowns->size > 0) {*/
  /*debug("There are still pending shutdowns\n");*/
  /*rc_msg.type = MPIDYNRES_RC_NONE;*/
  /*} else {*/
  /*switch (c->scheduling_mode) {*/
  /*case MPIDYNRES_MODE_RANDOM_DIFF: {*/
  /*random_diff_scheduling(scheduler, &rc_msg);*/
  /*break;*/
  /*}*/
  /*case MPIDYNRES_MODE_INC: {*/
  /*inc_scheduling(scheduler, &rc_msg);*/
  /*break;*/
  /*}*/
  /*case MPIDYNRES_MODE_INC_DEC: {*/
  /*inc_dec_scheduling(scheduler, &rc_msg);*/
  /*break;*/
  /*}*/
  /*}*/
  /*}*/

  /*// add rc to lookup table*/
  /*if (rc_msg.type != MPIDYNRES_RC_NONE) {*/
  /*rc_msg.tag = rc_table_add_entry(scheduler->rc_table, rc_msg);*/
  /*} else {*/
  /*rc_msg.tag = -1;*/
  /*}*/

  /*if (rc_msg.type == MPIDYNRES_RC_SUB) {*/
  /*// add to pending shutdowns*/
  /*MPIDYNRES_pset *set;*/
  /*MPIDYNRES_uri_table_lookup(scheduler->uri_table, rc_msg.uri, &set, &info);*/
  /*for (size_t i = 0; i < set->size; i++) {*/
  /*if (MPIDYNRES_pset_contains(scheduler->running_crs, set->cr_ids[i])) {*/
  /*MPIDYNRES_pset_add_cr(&scheduler->pending_shutdowns, set->cr_ids[i]);*/
  /*}*/
  /*}*/
  /*}*/
  /*// send anwer*/
  /*MPI_Send(&rc_msg, 1, get_rc_datatype(), status->MPI_SOURCE,*/
  /*MPIDYNRES_TAG_RC_ANSWER, scheduler->config->base_communicator);*/
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
void MPIDYNRES_scheduler_handle_rc_accept(
    MPIDYNRES_scheduler *scheduler, MPI_Status *status,
    MPIDYNRES_RC_accept_msg *rc_accept_msg) {
  // TODO
  /*MPIDYNRES_pset *set;*/
  /*MPI_Info info;*/

  /*(void)status;*/

  /*MPIDYNRES_RC_msg rc_msg =*/
  /*rc_table_get_entry(scheduler->rc_table, rc_accept_msg->rc_tag);*/
  /*if (rc_msg.tag == -1) {*/
  /*die("Inavalid rc_accept tag: %d, was already accepted or never created\n",*/
  /*rc_msg.tag);*/
  /*}*/
  /*rc_table_rem_entry(scheduler->rc_table, rc_accept_msg->rc_tag);*/

  /*if (rc_msg.type == MPIDYNRES_RC_ADD || rc_msg.type == MPIDYNRES_RC_REPLACE)
   * {*/
  /*MPIDYNRES_uri_table_lookup(scheduler->uri_table, rc_msg.uri, &set, &info);*/

  /*// insert init infos and start workers*/
  /*for (size_t i = 0; i < set->size; i++) {*/
  /*debug("Starting rank %d with passed uri %s init uri %s, init tag
   * %d\n",*/
  /*[>set->cr_ids[i], init_info.uri_passed, init_info.uri_init,<]*/
  /*[>init_info.init_tag);<]*/

  /*MPIDYNRES_scheduler_start_cr(scheduler, set->cr_ids[i]);*/
  /*}*/

  /*[> MPIDYNRES_pset_destroy(set); <]*/
  /*} else if (rc_msg.type == MPIDYNRES_RC_SUB) {*/
  /*MPIDYNRES_uri_table_lookup(scheduler->uri_table, rc_msg.uri, &set, &info);*/
  /*for (size_t i = 0; i < set->size; i++) {*/
  /*if (MPIDYNRES_pset_contains(scheduler->running_crs, set->cr_ids[i])) {*/
  /*set_state(set->cr_ids[i], accepted_shutdown);*/
  /*}*/
  /*}*/
  /*log_state("accepted shutdown of crs");*/
  /*}*/
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
void MPIDYNRES_scheduler_handle_pset_op(MPIDYNRES_scheduler *scheduler,
                                        MPI_Status *status,
                                        MPIDYNRES_pset_op_msg *pset_op_msg) {
  // TODO
  /*char res_uri[MPI_MAX_PSET_NAME_LEN] = {0};*/
  /*MPIDYNRES_pset *set1;*/
  /*MPIDYNRES_pset *set2;*/
  /*MPIDYNRES_pset *res_set;*/
  /*MPI_Info info;*/
  /*bool ok;*/

  /*ok = MPIDYNRES_uri_table_lookup(scheduler->uri_table, pset_op_msg->uri1,*/
  /*&set1, &info);*/
  /*if (!ok) {*/
  /*res_uri[0] = '\0';*/
  /*MPI_Send(res_uri, MPI_MAX_PSET_NAME_LEN, MPI_CHAR, status->MPI_SOURCE,*/
  /*MPIDYNRES_TAG_PSET_OP_ANSWER,*/
  /*scheduler->config->base_communicator);*/
  /*return;*/
  /*}*/
  /*ok = MPIDYNRES_uri_table_lookup(scheduler->uri_table, pset_op_msg->uri2,*/
  /*&set2, &info);*/
  /*if (!ok) {*/
  /*res_uri[0] = '\0';*/
  /*MPI_Send(res_uri, MPI_MAX_PSET_NAME_LEN, MPI_CHAR, status->MPI_SOURCE,*/
  /*MPIDYNRES_TAG_PSET_OP_ANSWER,*/
  /*scheduler->config->base_communicator);*/
  /*return;*/
  /*}*/
  /*res_set = MPIDYNRES_pset_from_set(set1);*/
  /*switch (pset_op_msg->op) {*/
  /*case MPIDYNRES_PSET_UNION: {*/
  /*MPIDYNRES_pset_union(&res_set, set2);*/
  /*break;*/
  /*}*/
  /*case MPIDYNRES_PSET_INTERSECT: {*/
  /*MPIDYNRES_pset_intersect(res_set, set2);*/
  /*break;*/
  /*}*/
  /*case MPIDYNRES_PSET_SUBTRACT: {*/
  /*MPIDYNRES_pset_subtract(res_set, set2);*/
  /*break;*/
  /*}*/
  /*}*/
  /*MPIDYNRES_uri_table_add_pset(scheduler->uri_table, res_set, MPI_INFO_NULL,*/
  /*res_uri);*/
  /*MPI_Send(res_uri, MPI_MAX_PSET_NAME_LEN, MPI_CHAR, status->MPI_SOURCE,*/
  /*MPIDYNRES_TAG_PSET_OP_ANSWER, scheduler->config->base_communicator);*/
}

void MPIDYNRES_scheduler_handle_session_create(MPIDYNRES_scheduler *scheduler,
                                               MPI_Status *status) {
  int err;
  err = MPI_Send(&scheduler->next_session_id, 1, MPI_INT, status->MPI_SOURCE,
                 MPIDYNRES_TAG_SESSION_CREATE_ANSWER,
                 scheduler->config->base_communicator);
  if (err) {
    die("Error in MPI_Send\n");
  }
  scheduler->next_session_id++;
}

void MPIDYNRES_scheduler_handle_session_info(MPIDYNRES_scheduler *scheduler,
                                             MPI_Status *status) {
  int cr_id = status->MPI_SOURCE;
  int err;
  char process_id_str[0x20] = {0};
  char origin_rc_tag_str[0x20] = {0};
  char *pending_shutdown_str;
  char *dynamic_start_str;
  MPI_Info info;

  set_process_state_node *psn = set_process_state_find(
      &scheduler->process_states, (process_state){.process_id = cr_id});

  snprintf(process_id_str, COUNT_OF(process_id_str) - 1, "%d",
           psn->key.process_id);
  pending_shutdown_str = psn->key.pending_shutdown ? "yes" : "no";
  dynamic_start_str = psn->key.dynamic_start ? "yes" : "no";
  snprintf(origin_rc_tag_str, COUNT_OF(origin_rc_tag_str) - 1, "%d",
           psn->key.origin_rc_tag);

  char const *const info_vec[] = {
      "mpidynres",
      "yes",
      "mpidynres_process_id",
      process_id_str,
      "mpidynres_pending_shutdown",
      pending_shutdown_str,
      "mpidynres_dynamic_start",
      dynamic_start_str,
      "mpidynres_origin_rc_tag",
      origin_rc_tag_str,
  };
  err = MPIDYNRES_Info_create_strings(COUNT_OF(info_vec), info_vec, &info);
  if (err) {
    die("Failed to create Info object\n");
  }

  err = MPIDYNRES_Send_MPI_Info(
      info, status->MPI_SOURCE, MPIDYNRES_TAG_SESSION_INFO_ANSWER_SIZE,
      MPIDYNRES_TAG_SESSION_INFO_ANSWER, scheduler->config->base_communicator);
  if (err) {
    die("Error in sending mpi info\n");
  }
  MPI_Info_free(&info);
}

// currently just do nothing
void MPIDYNRES_scheduler_handle_session_finalize(MPIDYNRES_scheduler *scheduler,
                                                 MPI_Status *status) {
  int err;
  int ok = 0;
  err = MPI_Send(&ok, 1, MPI_INT, status->MPI_SOURCE,
                 MPIDYNRES_TAG_SESSION_FINALIZE_ANSWER,
                 scheduler->config->base_communicator);
  if (err) {
    die("Error in MPI_Send\n");
  }
}

void MPIDYNRES_scheduler_handle_get_psets(MPIDYNRES_scheduler *scheduler,
                                          MPI_Status *status) {
  (void)scheduler;
  (void)status;
}

void MPIDYNRES_scheduler_handle_pset_info(MPIDYNRES_scheduler *scheduler,
                                          MPI_Status *status, int strsize) {
  // TODO
  /*int err;*/
  /*MPIDYNRES_pset *set;*/
  /*MPI_Info info;*/

  /*assert(strsize > 0);*/
  /*char *name = calloc(sizeof(char), strsize);*/

  /*err = MPI_Recv(name, strsize, MPI_CHAR, status->MPI_SOURCE,*/
  /*MPIDYNRES_TAG_PSET_INFO_NAME,*/
  /*scheduler->config->base_communicator, MPI_STATUS_IGNORE);*/
  /*if (err) {*/
  /*die("Error in MPI recv\n");*/
  /*}*/
  /*name[strsize] = '\0';*/
  /*bool in_there =*/
  /*MPIDYNRES_uri_table_lookup(scheduler->uri_table, name, &set, &info);*/

  /*if (!in_there) {*/
  /*info = MPI_INFO_NULL;*/
  /*}*/
  /*err = MPIDYNRES_Send_MPI_Info(*/
  /*info, status->MPI_SOURCE, MPIDYNRES_TAG_PSET_INFO_ANSWER_SIZE,*/
  /*MPIDYNRES_TAG_PSET_INFO_ANSWER, scheduler->config->base_communicator);*/
  /*if (err) {*/
  /*die("Error in sending mpi info\n");*/
  /*}*/
  }

void MPIDYNRES_scheduler_handle_pset_free(
  MPIDYNRES_scheduler *scheduler, MPI_Status *status,
  MPIDYNRES_pset_free_msg *pset_free_msg) {
  (void)pset_free_msg;
  (void)status;
  (void)scheduler;
  // TODO: mark as inactive
}

