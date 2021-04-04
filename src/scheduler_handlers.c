#include "logging.h"
#include "scheduler.h"
#include "string.h"
#include "util.h"

static int pset_free(MPIDYNRES_scheduler *scheduler, char *psetname) {
  assert(strlen(psetname) < MPI_MAX_PSET_NAME_LEN);
  debug("Removing process set %s\n", psetname);

  pset_node *psetn;

  // remove from scheduler map
  set_pset_node_find_by_name(&scheduler->pset_name_map, psetname, &psetn);

  if (psetn == NULL) {
    debug("Warning: Tried to free non-existing pset %s\n", psetname);
    return 1;
  }
  set_pset_node_erase(&scheduler->pset_name_map, *psetn);

  // remove from process states
  foreach (set_process_state, &scheduler->process_states, it) {
    pset_name *pnn;
    set_pset_name_find_by_name(&it.ref->psets_containing, psetname, &pnn);

    if (pnn != NULL) {
      set_pset_name_erase(&it.ref->psets_containing, *pnn);
    }
  }

  return 0;
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
  int cr_id = MPIDYNRES_scheduler_get_id_of_rank(status->MPI_SOURCE);

  if (set_int_count(&scheduler->running_crs, cr_id) != 1) {
    die("ERROR: expected %d to not run, but got worker done msg\n", cr_id);
  }

  // remove from pending shutdowns if in there
  if (set_int_count(&scheduler->pending_shutdowns, cr_id) == 1) {
    debug("Removing %d from pending shutdowns\n", cr_id);
    set_int_erase(&scheduler->pending_shutdowns, cr_id);
  }

  process_state *ps;
  set_process_state_find_by_id(&scheduler->process_states, cr_id, &ps);
  assert(ps != NULL);

  // create temporary copy as
  set_pset_name tmp = set_pset_name_copy(&ps->psets_containing);

  // remove all process sets containing the process
  foreach (set_pset_name, &tmp, it) { pset_free(scheduler, it.ref->name); }
  assert(ps->psets_containing.size == 0);

  set_pset_name_free(&tmp);

  // remove process state
  set_process_state_erase(&scheduler->process_states,
                          (process_state){.process_id = cr_id});

  // remove from running processes
  set_int_erase(&scheduler->running_crs, cr_id);

  set_state(cr_id, idle);
  log_state("cr id %d returned/exited", cr_id);
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
  int cr_id = MPIDYNRES_scheduler_get_id_of_rank(status->MPI_SOURCE);
  int err;
  char process_id_str[0x20] = {0};
  char origin_rc_tag_str[0x20] = {0};
  char *pending_shutdown_str;
  char *dynamic_start_str;
  MPI_Info info;
  process_state *ps;

  debug("In handle_session_info\n");

  set_process_state_find_by_id(&scheduler->process_states, cr_id, &ps);
  assert(ps != NULL);

  snprintf(process_id_str, COUNT_OF(process_id_str) - 1, "%d", ps->process_id);
  pending_shutdown_str = ps->pending_shutdown ? "yes" : "no";
  dynamic_start_str = ps->dynamic_start ? "yes" : "no";
  snprintf(origin_rc_tag_str, COUNT_OF(origin_rc_tag_str) - 1, "%d",
           ps->origin_rc_tag);

  if (ps->origin_rc_info == MPI_INFO_NULL) {
    MPI_Info_create(&info);
  } else {
    MPI_Info_dup(ps->origin_rc_info, &info);
  }

  // TODO: maybe add some scheduler/config info field
  MPI_Info_set(info, "mpidynres", "yes");
  MPI_Info_set(info, "mpidynres_process_id", process_id_str);
  MPI_Info_set(info, "mpidynres_pending_shutdown", pending_shutdown_str);
  MPI_Info_set(info, "mpidynres_dynamic_start", dynamic_start_str);
  MPI_Info_set(info, "mpidynres_origin_rc_tag", origin_rc_tag_str);

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
  int cr_id = MPIDYNRES_scheduler_get_id_of_rank(status->MPI_SOURCE);
  int err;
  MPI_Info psets_info;
  MPI_Info info;

  // receive mpi info
  err = MPIDYNRES_Recv_MPI_Info(
      &info, status->MPI_SOURCE, MPIDYNRES_TAG_GET_PSETS_INFO_SIZE,
      MPIDYNRES_TAG_GET_PSETS_INFO, scheduler->config->base_communicator,
      MPI_STATUS_IGNORE, MPI_STATUS_IGNORE);
  if (err) {
    die("Error in receiving mpi info\n");
  }
  // for now, we ignore it
  if (info != MPI_INFO_NULL) {
    MPI_Info_free(&info);
  }

  // create answer info object
  err = MPI_Info_create(&psets_info);
  if (err) {
    die("Error in MPI_Info_create\n");
  }

  process_state *ps;
  set_process_state_find_by_id(&scheduler->process_states, cr_id, &ps);

  foreach (set_pset_name, &ps->psets_containing, it) {
    char buf[0x10];
    snprintf(buf, COUNT_OF(buf), "%d", it.ref->pset_size);
    err = MPI_Info_set(psets_info, it.ref->name, buf);
    if (err) {
      die("Error in MPI_Info_set\n");
    }
  }

  err = MPI_Info_set(psets_info, "mpi://SELF", "1");
  if (err) {
    die("Error in MPI_Info_set\n");
  }

  err = MPIDYNRES_Send_MPI_Info(
      psets_info, status->MPI_SOURCE, MPIDYNRES_TAG_GET_PSETS_ANSWER_SIZE,
      MPIDYNRES_TAG_GET_PSETS_ANSWER, scheduler->config->base_communicator);
  if (err) {
    die("Error in sending mpi info\n");
  }
  MPI_Info_free(&psets_info);
}

void MPIDYNRES_scheduler_handle_pset_info(MPIDYNRES_scheduler *scheduler,
                                          MPI_Status *status, int strsize) {
  assert(strsize <= MPI_MAX_PSET_NAME_LEN);
  int err;
  MPI_Info pset_info;

  debug("In handle_pset_info\n");
  char *pset_name = calloc(strsize, sizeof(char));
  err = MPI_Recv(pset_name, strsize, MPI_CHAR, status->MPI_SOURCE,
                 MPIDYNRES_TAG_PSET_INFO_NAME,
                 scheduler->config->base_communicator, MPI_STATUS_IGNORE);
  if (err) {
    die("Error in mpi_recv\n");
  }
  debug("Info was requested for %s\n", pset_name);

  if (strcmp(pset_name, "mpi://SELF") == 0) {
    // special case of mpi://self
    char const *const info_vec[] = {
        "mpi_size",
        "1",
        "mpidynres_name",
        "mpi://SELF",
    };
    err =
        MPIDYNRES_Info_create_strings(COUNT_OF(info_vec), info_vec, &pset_info);
    if (err) {
      die("Failed to create Info object\n");
    }
  } else {
    pset_node *psetn;

    // remove from scheduler map
    set_pset_node_find_by_name(&scheduler->pset_name_map, pset_name, &psetn);

    if (psetn == NULL) {
      pset_info = MPI_INFO_NULL;
    } else {
      char buf[0x10];
      MPI_Info_dup(psetn->pset_info, &pset_info);
      snprintf(buf, COUNT_OF(buf), "%d", (int)psetn->pset.size);
      MPI_Info_set(pset_info, "mpi_size", buf);
      MPI_Info_set(pset_info, "mpidynres_name", psetn->pset_name);
    };
  }

  err = MPIDYNRES_Send_MPI_Info(
      pset_info, status->MPI_SOURCE, MPIDYNRES_TAG_PSET_INFO_ANSWER_SIZE,
      MPIDYNRES_TAG_PSET_INFO_ANSWER, scheduler->config->base_communicator);
  if (err) {
    die("Error in sending mpi info\n");
  }

  if (pset_info != MPI_INFO_NULL) {
    MPI_Info_free(&pset_info);
  }

  free(pset_name);
}

void MPIDYNRES_scheduler_handle_pset_free(
    MPIDYNRES_scheduler *scheduler, MPI_Status *status,
    MPIDYNRES_pset_free_msg *pset_free_msg) {
  (void)status;
  if (strcmp(pset_free_msg->pset_name, "mpi://SELF") == 0) {
    debug("Warning: Trying to free mpi://SELF");
  }
  int err;
  err = pset_free(scheduler, pset_free_msg->pset_name);
  if (err) {
    debug("Warning: Pset free failed\n");
  }
}

/**
 * @brief      handle a pset lookup message
 *
 * @details    look for the pset in the lookup table and send it to the
 * requesting rank
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
  // TODO: mpi://SELF
  int cr_id = MPIDYNRES_scheduler_get_id_of_rank(status->MPI_SOURCE);
  int err;
  size_t answer_size;
  debug("Lookup request from cr_id %d (strsize: %d)\n", cr_id, name_strsize);

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
  debug("%d wants to lookup %s\n", cr_id, name);

  pset_node *psetn;
  set_pset_node_find_by_name(&scheduler->pset_name_map, name, &psetn);
  free(name);

  bool in_there = (psetn != NULL);
  answer_size = in_there ? psetn->pset.size : 0;

  debug("Answer size: %zu\n", answer_size);

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
    foreach (set_int, &psetn->pset, it) {
      tmp[i++] = *it.ref;
      debug("%d\n", *it.ref);
    }
    MPI_Send(tmp, answer_size, MPI_INT, status->MPI_SOURCE,
             MPIDYNRES_TAG_PSET_LOOKUP_ANSWER,
             scheduler->config->base_communicator);
    free(tmp);
  } else {
    debug("Warning, cannot lookup pset\n");
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
void MPIDYNRES_scheduler_handle_pset_op(MPIDYNRES_scheduler *scheduler,
                                        MPI_Status *status,
                                        MPIDYNRES_pset_op_msg *pset_op_msg) {
  int cr_id = MPIDYNRES_scheduler_get_id_of_rank(status->MPI_SOURCE);

  int vlen, flag, err;
  char *pset_name1 = pset_op_msg->pset_name1;
  char *pset_name2 = pset_op_msg->pset_name2;
  char res_pset_name[MPI_MAX_PSET_NAME_LEN] = {0};
  bool random_name_choice = true;
  pset_node new_node;

  MPIDYNRES_pset_op op = pset_op_msg->op;
  MPI_Info info;

  // receive mpi info
  err = MPIDYNRES_Recv_MPI_Info(
      &info, status->MPI_SOURCE, MPIDYNRES_TAG_PSET_OP_INFO_SIZE,
      MPIDYNRES_TAG_PSET_OP_INFO, scheduler->config->base_communicator,
      MPI_STATUS_IGNORE, MPI_STATUS_IGNORE);
  if (err) {
    die("Error in receiving mpi info\n");
  }

  if (info != MPI_INFO_NULL) {
    MPI_Info_get_valuelen(info, "mpidynres_proposed_name", &vlen, &flag);
    if (flag && vlen < MPI_MAX_PSET_NAME_LEN) {
      MPI_Info_get(info, "mpidynres_proposed_name", vlen, res_pset_name, &flag);
      // check that name doesn't exist
      pset_node *pn;
      set_pset_node_find_by_name(&scheduler->pset_name_map, res_pset_name, &pn);
      if (pn == NULL && !MPIDYNRES_is_reserved_pset_name(res_pset_name)) {
        random_name_choice = false;
      }
    }
  }

  if (random_name_choice) {
    MPIDYNRES_gen_random_uri("mpidynres://op_", res_pset_name);
  }

  bool pn1_self = (strcmp(pset_name1, "mpi://SELF") == 0);
  bool pn2_self = (strcmp(pset_name2, "mpi://SELF") == 0);

  // get psets
  pset_node *pn1, *pn2;

  // create psets if mpi://SELF
  if (pn1_self) {
    pn1 = calloc(1, sizeof(pset_node));
    if (pn1 == NULL) {
      die("Memory Error!\n");
    }
    pn1->pset = set_int_init(int_compare);
    set_int_insert(&pn1->pset, cr_id);
    strcpy(pn1->pset_name, "mpi://SELF");
    pn1->pset_info = MPI_INFO_NULL;
  } else {
    set_pset_node_find_by_name(&scheduler->pset_name_map, pset_name1, &pn1);
  }

  if (pn2_self) {
    pn2 = calloc(1, sizeof(pset_node));
    if (pn2 == NULL) {
      die("Memory Error!\n");
    }
    pn2->pset = set_int_init(int_compare);
    set_int_insert(&pn2->pset, cr_id);
    strcpy(pn2->pset_name, "mpi://SELF");
    pn2->pset_info = MPI_INFO_NULL;
  } else {
    set_pset_node_find_by_name(&scheduler->pset_name_map, pset_name2, &pn2);
  }

  if (pn1 == NULL || pn2 == NULL) {
    debug(
        "Warning pset operation called with at least one invalid pset name "
        "%s "
        "%s\n",
        pset_name1, pset_name2);
    res_pset_name[0] = '\0';
  } else {
    debug("Creating new pset with name %s\n", res_pset_name);
    strcpy(new_node.pset_name, res_pset_name);
    if (info != MPI_INFO_NULL) {
      new_node.pset_info = info;
    } else {
      MPI_Info_create(&new_node.pset_info);
    }
    MPI_Info_set(new_node.pset_info, "mpidynres_op_parent1", pset_name1);
    MPI_Info_set(new_node.pset_info, "mpidynres_op_parent2", pset_name2);
    // TODO:
    /*MPI_Info_get_valuelen(info, "inherit_pset1_info", &vlen, &flag);*/
    /*MPI_Info_get_valuelen(info, "inherit_pset2_info", &vlen, &flag);*/
    switch (op) {
      case MPIDYNRES_PSET_UNION: {
        MPI_Info_set(new_node.pset_info, "mpidynres_op", "union");
        foreach (set_int, &pn1->pset, it) { printf("%d ", *it.ref); }
        printf("\nUNION\n");
        foreach (set_int, &pn2->pset, it) { printf("%d ", *it.ref); }
        printf("\n");
        new_node.pset = set_int_union(&pn1->pset, &pn2->pset);
        break;
      }
      case MPIDYNRES_PSET_INTERSECT: {
        MPI_Info_set(new_node.pset_info, "mpidynres_op", "intersect");
        new_node.pset = set_int_intersection(&pn1->pset, &pn2->pset);
        break;
      }
      case MPIDYNRES_PSET_DIFFERENCE: {
        MPI_Info_set(new_node.pset_info, "mpidynres_op", "difference");
        foreach (set_int, &pn1->pset, it) { printf("%d ", *it.ref); }
        printf("\nDIFF\n");
        foreach (set_int, &pn2->pset, it) { printf("%d ", *it.ref); }
        printf("\n");
        new_node.pset = set_int_difference(&pn1->pset, &pn2->pset);
        break;
      }
      default: {
        die("Warning: Unsupported pset operation %d\n", op);
        break;
      }
    }
  }

  if (new_node.pset.size == 0) {
    debug("Warning: Pset operation leads to empty result pset\n");
    set_int_free(&new_node.pset);
    res_pset_name[0] = '\0';
  }

  if (res_pset_name[0] != '\0') {
    pset_name pnn;
    strcpy(pnn.name, new_node.pset_name);
    pnn.pset_size = new_node.pset.size;

    foreach (set_int, &new_node.pset, it) {
      process_state *ps;
      set_process_state_find_by_id(&scheduler->process_states, *it.ref, &ps);
      if (ps != NULL) {
        // only for RC_SUB
        set_pset_name_insert(&ps->psets_containing, pnn);
      }
    }

    set_pset_node_insert(&scheduler->pset_name_map, new_node);
  }

  err = MPI_Send(res_pset_name, MPI_MAX_PSET_NAME_LEN, MPI_CHAR,
                 status->MPI_SOURCE, MPIDYNRES_TAG_PSET_OP_ANSWER,
                 scheduler->config->base_communicator);
  if (err) {
    die("Error in mpi_send\n");
  }

  // cleanup self sets
  if (pn1_self) {
    pset_node_free(pn1);
  }
  if (pn2_self) {
    pset_node_free(pn2);
  }
}

void MPIDYNRES_scheduler_handle_sched_hints(MPIDYNRES_scheduler *scheduler,
                                            MPI_Status *status,
                                            int session_id) {
  (void)session_id;
  int err;
  int cr_id = MPIDYNRES_scheduler_get_id_of_rank(status->MPI_SOURCE);
  MPI_Info hints_info;
  MPI_Info answer_info;

  err = MPIDYNRES_Recv_MPI_Info(
      &hints_info, status->MPI_SOURCE, MPIDYNRES_TAG_SCHED_HINTS_SIZE,
      MPIDYNRES_TAG_SCHED_HINTS_INFO, scheduler->config->base_communicator,
      MPI_STATUS_IGNORE, MPI_STATUS_IGNORE);
  if (err) {
    die("Error in receiving mpi info\n");
  }

  err = MPIDYNRES_manager_register_scheduling_hints(scheduler->manager, cr_id,
                                                    hints_info, &answer_info);
  if (err) {
    die("Error while registering scheduling hints\n");
  }

  err = MPIDYNRES_Send_MPI_Info(
      answer_info, status->MPI_SOURCE, MPIDYNRES_TAG_SCHED_HINTS_ANSWER_SIZE,
      MPIDYNRES_TAG_SCHED_HINTS_ANSWER, scheduler->config->base_communicator);
  if (err) {
    die("Error in sesnding mpi info\n");
  }

  MPI_Info_free(&hints_info);
  MPI_Info_free(&answer_info);
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
                                   MPI_Status *status, int session_id) {
  (void)session_id;
  MPIDYNRES_RC_msg rc_msg = {0};
  MPIDYNRES_RC_type rc_type;
  set_int new_pset;
  MPI_Info info = MPI_INFO_NULL;
  rc_info ri = {0};
  pset_node *pn;
  pset_node new_pset_node;
  pset_name pname;
  process_state *ps;
  char pset_name[MPI_MAX_PSET_NAME_LEN];
  char buf[0x100];
  int err;
  int cr_id = MPIDYNRES_scheduler_get_id_of_rank(status->MPI_SOURCE);
  char const *const rc_type_names[] = {
      [MPIDYNRES_RC_NONE] = "none",
      [MPIDYNRES_RC_ADD] = "add",
      [MPIDYNRES_RC_SUB] = "sub",
  };

  debug("RC Request from %d\n", cr_id);

  // if pending, shutdowns, return none
  if (scheduler->pending_shutdowns.size > 0) {
    debug("Warning: RC was requested, but there are still pending shutdowns\n");
    rc_type = MPIDYNRES_RC_NONE;
    info = MPI_INFO_NULL;
  } else {
    err = MPIDYNRES_manager_handle_rc_msg(scheduler->manager, cr_id, &info,
                                          &rc_type, &new_pset);
    if (err) {
      die("An error happened while trying to get resource changes");
    }
  }

  assert(rc_type != MPIDYNRES_RC_NONE || info == MPI_INFO_NULL);

  if (rc_type != MPIDYNRES_RC_NONE) {
    ri.rc_tag = scheduler->next_rc_tag;
    scheduler->next_rc_tag += 1;
    // create new pset name
    snprintf(pset_name, COUNT_OF(pset_name), "mpidynres://rc_%d", ri.rc_tag);

    // create rc_info and insert into lookup set
    ri.rc_tag = scheduler->next_rc_tag;
    ri.rc_type = rc_type;
    ri.pset = set_int_copy(&new_pset);
    strcpy(ri.new_pset_name, pset_name);
    set_rc_info_insert(&scheduler->rc_map, ri);
    scheduler->next_rc_tag += 1;

    // create new pset_node
    new_pset_node.pset = new_pset;
    strcpy(new_pset_node.pset_name, pset_name);

    MPI_Info_create(&new_pset_node.pset_info);
    MPI_Info_set(new_pset_node.pset_info, "mpidynres_rc", "true");
    MPI_Info_set(new_pset_node.pset_info, "mpidynres_rc_type",
                 rc_type_names[ri.rc_type]);
    MPI_Info_set(new_pset_node.pset_info, "mpidynres_name", pset_name);
    snprintf(buf, COUNT_OF(buf), "%ld", new_pset.size);
    MPI_Info_set(new_pset_node.pset_info, "mpi_size", buf);
    snprintf(buf, COUNT_OF(buf), "%d", ri.rc_tag);
    MPI_Info_set(new_pset_node.pset_info, "mpidynres_rc_tag", buf);

    // insert into pset name map
    set_pset_node_insert(&scheduler->pset_name_map, new_pset_node);

    // update psets_containing
    /*
     * After this we have a small inconsistency:
     * In the case of an MPIDYNRES_RC_ADD
     * there are processes in psets that are tracked in the scheduler's
     * pset_name_map, but they are not in the process_state map
     * This consistency can be widened if operations etc happen on the pset
     *
     * TODO: either, we add them and mark them reserved and add checks
     * or we let the inconsistency be and fix it, when rc_accept is called
     */
    strcpy(pname.name, pset_name);
    pname.pset_size = new_pset.size;
    foreach (set_int, &new_pset, it) {
      set_process_state_find_by_id(&scheduler->process_states, *it.ref, &ps);
      if (ps != NULL) {
        // should only  be here in case of RC_SUB
        set_pset_name_insert(&ps->psets_containing, pname);
      }
    }
  }

  if (rc_type == MPIDYNRES_RC_SUB) {
    // add to pending shutdowns
    set_int_free(&scheduler->pending_shutdowns);
    set_pset_node_find_by_name(&scheduler->pset_name_map, pset_name, &pn);
    assert(pn != NULL);
    scheduler->pending_shutdowns = set_int_copy(&pn->pset);

    // update process states
    foreach (set_int, &pn->pset, it) {
      set_process_state_find_by_id(&scheduler->process_states, *it.ref, &ps);
      assert(ps != NULL);
      ps->pending_shutdown = true;
    }
    // update logging state
    foreach (set_int, &pn->pset, it) { set_state(*it.ref, proposed_shutdown); }
    log_state("proposing to shutdown crs");
  } else if (rc_type == MPIDYNRES_RC_ADD) {
    set_pset_node_find_by_name(&scheduler->pset_name_map, pset_name, &pn);
    assert(pn != NULL);
    // update logging statee
    foreach (set_int, &pn->pset, it) { set_state(*it.ref, reserved); }
    log_state("proposing to start crs");
  }

  // create rc_msg
  if (rc_type == MPIDYNRES_RC_NONE) {
    rc_msg.tag = -1;
    rc_msg.type = -1;
  } else {
    strcpy(rc_msg.pset_name, pset_name);
    rc_msg.tag = ri.rc_tag;
    rc_msg.type = rc_type;
  }

  // send info answer
  err = MPIDYNRES_Send_MPI_Info(
      info, status->MPI_SOURCE, MPIDYNRES_TAG_RC_INFO_SIZE,
      MPIDYNRES_TAG_RC_INFO, scheduler->config->base_communicator);
  if (err) {
    die("Error in receiving mpi info\n");
  }
  if (info != MPI_INFO_NULL) {
    MPI_Info_free(&info);
  }

  // send rc_msg
  err = MPI_Send(&rc_msg, 1, get_rc_datatype(), status->MPI_SOURCE,
                 MPIDYNRES_TAG_RC_ANSWER, scheduler->config->base_communicator);
  if (err) {
    die("Error in sending rc reply\n");
  }

  // update pending
  scheduler->pending_resource_change = true;
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
 * @param      rc_accept_msg the resource change accept message struct with
 * more information about the rc
 */
void MPIDYNRES_scheduler_handle_rc_accept(MPIDYNRES_scheduler *scheduler,
                                          MPI_Status *status, int session_id,
                                          int rc_tag) {
  (void)session_id;
  rc_info *ri;

  MPI_Info info = MPI_INFO_NULL, origin_rc_info = MPI_INFO_NULL;
  int err;
  int cr_id = MPIDYNRES_scheduler_get_id_of_rank(status->MPI_SOURCE);

  debug("RC Accept from %d\n", cr_id);

  err = MPIDYNRES_Recv_MPI_Info(
      &info, status->MPI_SOURCE, MPIDYNRES_TAG_RC_ACCEPT_INFO_SIZE,
      MPIDYNRES_TAG_RC_ACCEPT_INFO, scheduler->config->base_communicator,
      MPI_STATUS_IGNORE, MPI_STATUS_IGNORE);
  if (err) {
    die("Error in receiving mpi info\n");
  }

  assert(scheduler->pending_resource_change);

  set_rc_info_find_by_tag(&scheduler->rc_map, rc_tag, &ri);
  if (ri == NULL) {
    die("Invalid rc_accept tag: %d, was already accepted or never created\n",
        rc_tag);
  }

  switch (ri->rc_type) {
    case MPIDYNRES_RC_ADD: {
      // start new crs
      if (info == MPI_INFO_NULL) {
        origin_rc_info = MPI_INFO_NULL;
        foreach (set_int, &ri->pset, it) {
          MPIDYNRES_scheduler_start_cr(scheduler, *it.ref, true, rc_tag,
                                       origin_rc_info);
        }
      } else {
        foreach (set_int, &ri->pset, it) {
          MPI_Info_dup(info, &origin_rc_info);
          MPIDYNRES_scheduler_start_cr(scheduler, *it.ref, true, rc_tag,
                                       origin_rc_info);
        }
        MPI_Info_free(&info);
      }
      break;
    }
    case MPIDYNRES_RC_SUB: {
      // update logging state
      foreach (set_int, &ri->pset, it) {
        if (set_int_find(&scheduler->running_crs, *it.ref) != NULL) {
          set_state(*it.ref, accepted_shutdown);
        }
      }
      break;
    }
    default: {
      die("Unkown rc type saved: %d\n", ri->rc_type);
      break;
    }
  }

  // update pending
  scheduler->pending_resource_change = false;

  set_rc_info_erase(&scheduler->rc_map, *ri);
}
