/*
 * Implementation of the functions called by the simulated processes
 */
#include "mpidynres.h"

#include <mpi.h>
#include <setjmp.h>
#include <stdint.h>
#include <string.h>

#include "comm.h"
#include "logging.h"
#include "util.h"

MPI_Session MPI_SESSION_INVALID = NULL;

MPI_Comm g_MPIDYNRES_base_comm;  // store the base communicator

jmp_buf g_MPIDYNRES_JMP_BUF;  // store correct return position of simulation

/**
 * @brief  Exit from simulated process
 *
 * @details  real process will idle until all exit or its started again
 */
void MPIDYNRES_exit() {
  debug("MPIDYNRES_exit called");
  longjmp(g_MPIDYNRES_JMP_BUF, 1);
}

int MPI_Session_init(MPI_Info info, MPI_Errhandler errhandler,
                     MPI_Session *session) {
  (void)errhandler;
  (void)info;
  int unused;
  int err;
  struct internal_mpi_session *sess =
      calloc(sizeof(struct internal_mpi_session), 1);
  if (!sess) {
    die("Memory error\n");
  }

  err = MPI_Send(&unused, 1, MPI_INT, 0, MPIDYNRES_TAG_SESSION_CREATE,
                 g_MPIDYNRES_base_comm);
  if (err) {
    return err;
  }
  err = MPI_Recv(&sess->session_id, 1, MPI_INT, 0,
                 MPIDYNRES_TAG_SESSION_CREATE_ANSWER, g_MPIDYNRES_base_comm,
                 MPI_STATUS_IGNORE);
  if (err) {
    return err;
  }
  if (sess->session_id == MPIDYNRES_INVALID_SESSION_ID) {
    *session = MPI_SESSION_INVALID;
    free(sess);
  } else {
    *session = sess;
  }
  return 0;
}

int MPI_Session_finalize(MPI_Session *session) {
  int err;
  int answer;
  if (*session == MPI_SESSION_INVALID) {
    debug("Warning: MPI_Session_finalize called with MPI_SESSION_INVALID\n");
    return 0;
  }
  err = MPI_Send(&((*session)->session_id), 1, MPI_INT, 0,
                 MPIDYNRES_TAG_SESSION_FINALIZE, g_MPIDYNRES_base_comm);
  if (err) {
    return err;
  }
  err = MPI_Recv(&answer, 1, MPI_INT, 0, MPIDYNRES_TAG_SESSION_FINALIZE_ANSWER,
                 g_MPIDYNRES_base_comm, MPI_STATUS_IGNORE);
  if (err) {
    return err;
  }
  free(*session);
  *session = MPI_SESSION_INVALID;
  return answer;
}

int MPI_Session_get_info(MPI_Session session, MPI_Info *info_used) {
  int err;
  MPI_Info info;
  if (session == MPI_SESSION_INVALID) {
    debug("Warning: MPI_Session_finalize called with MPI_SESSION_INVALID\n");
    *info_used = MPI_INFO_NULL;
    return 1;
  }
  err = MPI_Send(&(session->session_id), 1, MPI_INT, 0,
                 MPIDYNRES_TAG_SESSION_INFO, g_MPIDYNRES_base_comm);
  if (err) {
    return err;
  }
  err = MPIDYNRES_Recv_MPI_Info(
      &info, 0, MPIDYNRES_TAG_SESSION_INFO_ANSWER_SIZE,
      MPIDYNRES_TAG_SESSION_INFO_ANSWER, g_MPIDYNRES_base_comm,
      MPI_STATUS_IGNORE, MPI_STATUS_IGNORE);
  if (err) {
    return err;
  }
  *info_used = info;
  return 0;
}

/*int MPI_Session_get_num_psets(MPI_Session session, MPI_Info info,*/
/*int *npset_name) {*/
/*int err;*/
/*if (session == MPI_SESSION_INVALID) {*/
/*debug(*/
/*"Warning: MPI_Session_get_num_psets called with MPI_SESSION_INVALID\n");*/
/*return 1;*/
/*}*/
/*err = MPI_Send(&session->session_id, 1, MPI_INT, 0, MPIDYNRES_TAG_NUM_PSETS,*/
/*g_MPIDYNRES_base_comm);*/
/*if (err) {*/
/*return err;*/
/*}*/
/*err = MPIDYNRES_Send_MPI_Info(info, 0, MPIDYNRES_TAG_NUM_PSETS_INFO_SIZE,*/
/*MPIDYNRES_TAG_NUM_PSETS_INFO,*/
/*g_MPIDYNRES_base_comm);*/
/*if (err) {*/
/*return err;*/
/*}*/
/*err = MPI_Recv(npset_name, 1, MPI_INT, 0, MPIDYNRES_TAG_NUM_PSETS_ANSWER,*/
/*g_MPIDYNRES_base_comm, MPI_STATUS_IGNORE);*/
/*return err;*/
/*}*/

/*int MPI_Session_get_nth_pset(MPI_Session session, MPI_Info info, int n,*/
/*int *pset_len, char *pset_name) {*/
/*int err;*/
/*int msg[3];*/
/*MPI_Status status;*/

/*if (session == MPI_SESSION_INVALID) {*/
/*debug(*/
/*"Warning: MPI_Session_get_nth_pset called with MPI_SESSION_INVALID\n");*/
/*return 1;*/
/*}*/
/*if (*pset_len < 1 || pset_name == NULL) {*/
/*debug("Warning: bad parameters for call of MPI_Session_get_nth_pset\n");*/
/*return 1;*/
/*}*/
/*msg[0] = session->session_id;*/
/*msg[1] = n;*/
/*msg[2] = *pset_len;*/
/*err = MPI_Send(msg, 3, MPI_INT, 0, MPIDYNRES_TAG_NTH_PSET,*/
/*g_MPIDYNRES_base_comm);*/
/*if (err) {*/
/*return err;*/
/*}*/
/*err = MPIDYNRES_Send_MPI_Info(info, 0, MPIDYNRES_TAG_NTH_PSET_INFO_SIZE,*/
/*MPIDYNRES_TAG_NTH_PSET_INFO,*/
/*g_MPIDYNRES_base_comm);*/
/*if (err) {*/
/*return err;*/
/*}*/
/*// pset len includes the zero byte*/
/*err = MPI_Recv(pset_name, *pset_len, MPI_CHAR, 0,*/
/*MPIDYNRES_TAG_NTH_PSET_ANSWER, g_MPIDYNRES_base_comm, &status);*/
/*if (err) {*/
/*return err;*/
/*}*/
/*if (pset_name[0] == '\0') {*/
/**pset_len = -1;*/
/*return 1;*/
/*}*/
/*err = MPI_Get_count(&status, MPI_CHAR, pset_len);*/
/*if (err) {*/
/*return err;*/
/*}*/
/*return 0;*/
/*}*/

int MPI_Session_get_psets(MPI_Session session, MPI_Info info, MPI_Info *psets) {
  int err;
  if (session == MPI_SESSION_INVALID) {
    debug("Warning: MPI_Session_get_psets called with MPI_SESSION_INVALID\n");
    return 1;
  }
  err = MPI_Send(&session->session_id, 1, MPI_INT, 0, MPIDYNRES_TAG_GET_PSETS,
                 g_MPIDYNRES_base_comm);
  if (err) {
    return err;
  }
  err = MPIDYNRES_Send_MPI_Info(info, 0, MPIDYNRES_TAG_GET_PSETS_INFO_SIZE,
                                MPIDYNRES_TAG_GET_PSETS_INFO,
                                g_MPIDYNRES_base_comm);
  if (err) {
    return err;
  }
  err = MPIDYNRES_Recv_MPI_Info(psets, 0, MPIDYNRES_TAG_GET_PSETS_ANSWER_SIZE,
                                MPIDYNRES_TAG_GET_PSETS_ANSWER,
                                g_MPIDYNRES_base_comm, MPI_STATUS_IGNORE,
                                MPI_STATUS_IGNORE);
  if (err) {
    return err;
  }

  return 0;
}

int MPI_Session_get_pset_info(MPI_Session session, char const *pset_name,
                              MPI_Info *info) {
  int err;
  int msg[2];

  if (session == MPI_SESSION_INVALID) {
    debug(
        "Warning: MPI_Session_get_pset_info called with MPI_SESSION_INVALID\n");
    return 1;
  }
  if (pset_name == NULL) {
    debug("Warning: MPI_Session_get_pset_info called with NULL name\n");
    return 1;
  }
  msg[0] = session->session_id;
  msg[1] = strlen(pset_name) + 1;
  assert(msg[1] <= MPI_MAX_PSET_NAME_LEN);
  err = MPI_Send(msg, 2, MPI_INT, 0, MPIDYNRES_TAG_PSET_INFO,
                 g_MPIDYNRES_base_comm);
  if (err) {
    return err;
  }
  err = MPI_Send(pset_name, msg[1], MPI_CHAR, 0, MPIDYNRES_TAG_PSET_INFO_NAME,
                 g_MPIDYNRES_base_comm);
  if (err) {
    return err;
  }
  err = MPIDYNRES_Recv_MPI_Info(info, 0, MPIDYNRES_TAG_PSET_INFO_ANSWER_SIZE,
                                MPIDYNRES_TAG_PSET_INFO_ANSWER,
                                g_MPIDYNRES_base_comm, MPI_STATUS_IGNORE,
                                MPI_STATUS_IGNORE);
  if (err) {
    return err;
  }
  return 0;
}

int MPI_Group_from_session_pset(MPI_Session session, char const *pset_name,
                                MPI_Group *newgroup) {
  int err;
  int msg[2];
  size_t answer_size;
  MPI_Group base_group = {0};

  if (session == MPI_SESSION_INVALID) {
    debug("Warning: MPI_Group_from_session_pset called with invalid session\n");
    *newgroup = MPI_GROUP_EMPTY;
    return 0;
  }
  if (pset_name == NULL) {
    debug("Warning: MPI_Group_from_session_pset called with NULL pset_name\n");
    return 1;
  }

  msg[0] = session->session_id;
  msg[1] = strlen(pset_name) + 1;
  assert(msg[1] < MPI_MAX_PSET_NAME_LEN);
  err = MPI_Send(msg, 2, MPI_INT, 0, MPIDYNRES_TAG_PSET_LOOKUP,
                 g_MPIDYNRES_base_comm);
  if (err) {
    return err;
  }
  err = MPI_Send(pset_name, msg[1], MPI_CHAR, 0, MPIDYNRES_TAG_PSET_LOOKUP_NAME,
                 g_MPIDYNRES_base_comm);
  if (err) {
    return err;
  }
  err = MPI_Recv(&answer_size, 1, my_MPI_SIZE_T, 0,
                 MPIDYNRES_TAG_PSET_LOOKUP_ANSWER_SIZE, g_MPIDYNRES_base_comm,
                 MPI_STATUS_IGNORE);
  if (err) {
    return err;
  }

  if (answer_size == 0) {
    debug("Warning: THere was a problem getting the set\n");
    return 1;
  }

  int *cr_ids = calloc(answer_size, sizeof(int));
  if (cr_ids == NULL) {
    die("Memory Error\n");
  }
  MPI_Recv(cr_ids, answer_size, MPI_INT, 0, MPIDYNRES_TAG_PSET_LOOKUP_ANSWER,
           g_MPIDYNRES_base_comm, MPI_STATUS_IGNORE);

  err = MPI_Comm_group(g_MPIDYNRES_base_comm, &base_group);
  if (err) {
    debug("Failed to create mpi group for base communicator\n");
    free(cr_ids);
    MPI_Group_free(&base_group);
    return err;
  }
  if (base_group == MPI_GROUP_EMPTY) {
    debug("Couldn't create mpi group from base communicator\n");
    free(cr_ids);
    MPI_Group_free(&base_group);
    return err;
  }
  
  /*BREAK();*/
  err = MPI_Group_incl(base_group, answer_size, cr_ids, newgroup);
  if (err) {
    debug("MPI_Group_incl failed\n");
    free(cr_ids);
    MPI_Group_free(&base_group);
    return err;
  }

  MPI_Group_free(&base_group);
  free(cr_ids);
  return 0;
}


int MPI_Comm_create_from_group(MPI_Group group, char *const stringtag, MPI_Info info, MPI_Errhandler errhandler, MPI_Comm *newcomm) {
  (void) stringtag;
  (void) info;
  (void) errhandler;
  int err = MPI_Comm_create_group(g_MPIDYNRES_base_comm, group, 0, newcomm);
  if (err) {
    debug("MPI_Comm_create_failed");
    return err;
  }
  return 0;
}


/**
 * @brief      ask the scheduler for a new pset that contains the contents
of
a
 * set operation applied on two psets
 *
 * @param      i_uri1 the first uri of the operation
 *
 * @param      i_uri2 the second uri of the opertaion
 *
 * @param      i_op the operation that should be performed
 *
 * @param      o_uri_result the array where the resulting uri should be
written
 * to
 *
 * @return     if != 0, an error has happened
 */
int MPIDYNRES_pset_create_op(MPI_Session session, char const i_pset_name1[],
                             char const i_pset_name2[], MPIDYNRES_pset_op i_op,
                             char o_pset_result_name[MPI_MAX_PSET_NAME_LEN]) {
  int err;
  MPIDYNRES_pset_op_msg msg = {0};
  msg.session_id = session->session_id;
  msg.op = i_op;
  strncpy(msg.uri1, i_pset_name1, MPI_MAX_PSET_NAME_LEN - 1);
  strncpy(msg.uri2, i_pset_name2, MPI_MAX_PSET_NAME_LEN - 1);
  err = MPI_Send(&msg, 1, get_pset_op_datatype(), 0, MPIDYNRES_TAG_PSET_OP,
                 g_MPIDYNRES_base_comm);
  if (err) {
    return err;
  }
  err = MPI_Recv(o_pset_result_name, MPI_MAX_PSET_NAME_LEN, MPI_CHAR, 0,
                 MPIDYNRES_TAG_PSET_OP_ANSWER, g_MPIDYNRES_base_comm,
                 MPI_STATUS_IGNORE);
  if (err) {
    return err;
  }
  return 0;
}

/**
 * @brief      tell the resource manager that a pset can be freed
 *
 * @param      io_uri the uri that can be freed
 *
 * @return     if != 0, an error has happened
 */
int MPIDYNRES_pset_free(MPI_Session session,
                        char i_pset_name[MPI_MAX_PSET_NAME_LEN]) {
  int err;
  struct MPIDYNRES_pset_free_msg msg = {0};
  msg.session_id = session->session_id;
  strncpy(msg.pset_name, i_pset_name, MPI_MAX_PSET_NAME_LEN - 1);
  err = MPI_Send(&msg, 1, get_pset_free_datatype(), 0, MPIDYNRES_TAG_PSET_FREE,
                 g_MPIDYNRES_base_comm);
  if (err) {
    return err;
  }
  return 0;
}

/**
 * @brief      ask the scheduler about resource changes
 *
 * @param      o_rc_type the rc_type (result)
 *
 * @param      o_diff_uri the uri of the delta set (only if no RC_NONE)
(result)
 *
 * @param      o_tag the tag/handle for the reosurce change that can be
used
 * with RC_accept
 *
 * @return     if != 0, an error has happened
 */
int MPIDYNRES_RC_fetch(MPI_Session session, MPIDYNRES_RC_type *o_rc_type,
                       char o_diff_pset_name[MPI_MAX_PSET_NAME_LEN],
                       MPIDYNRES_RC_tag *o_tag) {
  int err;
  MPIDYNRES_RC_msg answer = {0};

  err = MPI_Ssend(&session->session_id, 1, MPI_INT, 0, MPIDYNRES_TAG_RC,
                  g_MPIDYNRES_base_comm);
  if (err) {
    return err;
  }
  err = MPI_Recv(&answer, 1, get_rc_datatype(), 0, MPIDYNRES_TAG_RC_ANSWER,
                 g_MPIDYNRES_base_comm, MPI_STATUS_IGNORE);
  if (err) {
    return err;
  }
  strcpy(o_diff_pset_name, answer.uri);
  *o_rc_type = answer.type;
  *o_tag = answer.tag;
  return 0;
}

/**
 * @brief      accept a resource change
 *
 * @details    TODO
 *
 * @param      i_rc_tag the tag of the resource change to accept
 *
 * @param      i_new_process_tag a tag that should be passed to all newly
 * starting processes
 *
 * @param      i_uri_msg the uri that shuld be passed to all newly starting
 * processes
 *
 * @return     if != 0, an error has happened
 */
int MPIDYNRES_RC_accept(MPI_Session session, MPIDYNRES_RC_tag i_rc_tag,
                        MPI_Info info) {
  int err;
  int msg[2];
  int answer;
  msg[0] = session->session_id;
  msg[1] = i_rc_tag;
  err = MPI_Send(&msg, 2, MPI_INT, 0, MPIDYNRES_TAG_RC_ACCEPT,
                 g_MPIDYNRES_base_comm);
  if (err) {
    return err;
  }
  err = MPIDYNRES_Send_MPI_Info(info, 0, MPIDYNRES_TAG_RC_ACCEPT_INFO_SIZE,
                                MPIDYNRES_TAG_RC_ACCEPT_INFO,
                                g_MPIDYNRES_base_comm);
  if (err) {
    return err;
  }
  err = MPI_Send(&answer, 1, MPI_INT, 0, MPIDYNRES_TAG_RC_ACCEPT_ANSWER,
                 g_MPIDYNRES_base_comm);
  if (err) {
    return err;
  }
  return answer;
}

int MPIDYNRES_Info_create_strings(size_t kvlist_size,
                                  char const *const kvlist[], MPI_Info *info) {
  int res;

  if (kvlist_size % 2 != 0) {
    debug(
        "Warning: Tried to create MPI_Info object from kvlist with uneven "
        "size");
    return 1;
  }
  res = MPI_Info_create(info);
  if (res) {
    debug("Failed to create MPI_Info object\n");
    return res;
  }
  for (size_t i = 0; i < kvlist_size; i += 2) {
    char const *key = kvlist[i];
    char const *val = kvlist[i + 1];
    res = MPI_Info_set(*info, key, val);
    if (res) {
      *info = MPI_INFO_NULL;
      debug("Failed to set stuff in the MPI_Info object\n");
      return res;
    }
  }

  return 0;
}
