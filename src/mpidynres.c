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

MPI_Session MPI_SESSION_NULL = NULL;

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

/**
 * @brief      Initialize an MPI Sessions object
 *
 * @details    The MPI_Session_init function is used to create a new
MPI\_Session object. It should be called at the beginning of the application
before any MPI communication takes place. Multiple session objects can be
constructed and nested. However, the behavior of the functions in this header
will not differ between the session objects.
 *
 * @param      info An info object containing hints that will be associated with
the Session (argument has to be freed by caller)
 *
 * @param      errhandler Will be ignored, for API compliance
 *
 * @param      session The new session will be returned in this object
 *
 * @return     If != 0, an error occured
 */
int MPI_Session_init(MPI_Info info, MPI_Errhandler errhandler,
                     MPI_Session *session) {
  (void)errhandler;
  int unused = 0;
  int err;
  debug("In MPI_Session_init\n");

  debug("allocating internal mpi session with %zu bytes\n",
        sizeof(struct internal_mpi_session));

  struct internal_mpi_session *sess =
      calloc(1, sizeof(struct internal_mpi_session));

  if (!sess) {
    die("Memory error\n");
  }

  err = MPI_Send(&unused, 1, MPI_INT, 0, MPIDYNRES_TAG_SESSION_CREATE,
                 g_MPIDYNRES_base_comm);
  if (err) {
    debug("Warning: Failed to send session create\n");
    free(sess);
    return err;
  }
  err = MPI_Recv(&sess->session_id, 1, MPI_INT, 0,
                 MPIDYNRES_TAG_SESSION_CREATE_ANSWER, g_MPIDYNRES_base_comm,
                 MPI_STATUS_IGNORE);
  if (err) {
    debug("Warning: Failed to recv session id\n");
    free(sess);
    return err;
  }
  if (info == MPI_INFO_NULL) {
    sess->info = MPI_INFO_NULL;
  } else {
    err = MPI_Info_dup(info, &sess->info);
    if (err) {
      debug("Warning: Failed to dup info\n");
      free(sess);
      return err;
    }
  }
  if (sess->session_id == MPIDYNRES_INVALID_SESSION_ID) {
    *session = MPI_SESSION_NULL;
    debug("Warning: Got Invalid session id\n");
    if (sess->info != MPI_INFO_NULL) {
      MPI_Info_free(&sess->info);
    }
    free(sess);
    return 1;
  } else {
    *session = sess;
    return 0;
  }
}

/**
 * @brief      Finalize an MPI Session
 *
 * @details    The MPI_Session_finalize is used to destruct an MPI session and
 * has to be called on each constructed session object before a process exits.
 * It frees the session argument and replaces it with MPI_SESSION_NULL. The
 * function call is not collective.
 *
 * @param      session The session object to free
 *
 * @return     if != 0, an error occured
 */
int MPI_Session_finalize(MPI_Session *session) {
  int err;
  int answer;
  debug("In MPI_Session_finalize\n");
  if (*session == MPI_SESSION_NULL) {
    debug("Warning: MPI_Session_finalize called with MPI_SESSION_NULL\n");
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
  if ((*session)->info != MPI_INFO_NULL) {
    MPI_Info_free(&((*session)->info));
  }
  free(*session);
  *session = MPI_SESSION_NULL;
  return answer;
}

/**
 * @brief      Get info object associated with MPI Session
 *
 * @details    MPI_Session_get_info is used to obtain hints associated with the
 * session argument.
 * A new MPI_Info object is created that contains key-value pairs and returned
 * in the info_used argument.  The info object has to be freed by the
 * application. All key-value pairs passed to the initialization function stay
 * associated with the session object and will be contained in the info object
 * returned by this function. This can prove useful for coordination among
 * different software components sharing the same MPI process.
 *
 * @param      session The session used
 *
 * @param      info_used The info object will be returned in this argument
 *
 * @return     if != 0, an error occured
 */
int MPI_Session_get_info(MPI_Session session, MPI_Info *info_used) {
  int err;
  MPI_Info info;
  if (session == MPI_SESSION_NULL) {
    debug("Warning: MPI_Session_finalize called with MPI_SESSION_NULL\n");
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
  if (info == MPI_INFO_NULL) {
    printf("Hallo\n");
    // the mpi-4.0 draft says that we need to return an empty info object if
    // there are no keys
  }

  // merge received info with session->info
  if (session->info != MPI_INFO_NULL) {
    {
      int nkeys, unused, contains;
      char key[MPI_MAX_INFO_KEY];
      char val[MPI_MAX_INFO_VAL];
      MPI_Info_get_nkeys(session->info, &nkeys);
      for (int i = 0; i < nkeys; i++) {
        // get key
        MPI_Info_get_nthkey(session->info, i, key);

        MPI_Info_get_valuelen(info, key, &unused, &contains);
        // Info object from resource manager has higher priority
        if (!contains) {
          // get value
          MPI_Info_get(session->info, key, MPI_MAX_INFO_VAL - 1, val, &unused);
          // set key,value in info
          MPI_Info_set(info, key, val);
        }
      }
    }
  }

  *info_used = info;
  return 0;
}

/**
 * @brief      Return all process sets that the calling process is part of
 *
 * @details    The MPI_Session_get_psets function is used to query process sets
 * that the calling process is part of. The info argument can be used to add
 * hints to the query. Currently, no hints are supported yet. The resulting
 * process sets will be available in a new MPI_Info object in the psets argument
 * which must be freed by the application. The keys of the MPI_Info object are
 * the names of the process sets and the corresponding value is a string
 * containing the decimal representation of the process set size , i.e. the
 * number of processes contained in the process set. The string can be converted
 * to an integer using the atoi function from the C standard library.
 *
 * @param      session The session used
 *
 * @param      info An info object to add hints to the query
 *
 * @param      psets The process sets are returned in this argument. The keys
 * are the process set names and the value the decimal representation of the
 * size of the process set
 *
 * @return     if != 0, an error occured
 */
int MPI_Session_get_psets(MPI_Session session, MPI_Info info, MPI_Info *psets) {
  int err;
  if (session == MPI_SESSION_NULL) {
    debug("Warning: MPI_Session_get_psets called with MPI_SESSION_NULL\n");
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

/**
 * @brief      Get info object associated with process set
 *
 * @details    This function allows the application to query for properties of a
 * process sets. The pset argument contains the name of the process set to
 * query. The property will be returned in the info argument which has to be
 * freed by the application.
 *
 * @param      session The session used
 *
 * @param      pset_name The name of the process set
 *
 * @param      info The info object is returned in this argument
 *
 * @return     if != 0, an error occured
 */
int MPI_Session_get_pset_info(MPI_Session session, char const *pset_name,
                              MPI_Info *info) {
  int err;
  int msg[2];

  if (session == MPI_SESSION_NULL) {
    debug("Warning: MPI_Session_get_pset_info called with MPI_SESSION_NULL\n");
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

/**
 * @brief      Create MPI_Group from a process set
 *
 * @details    MPI_Group_from_session_pset is used to create a new
 * group containing the processes in the process set with the name passed in the
 * pset argument. If an error occurs, group will be set to
 * MPI_GROUP_EMPTY and a non-zero return value are returned.
 *
 * @param      session The session used
 *
 * @param      pset_name The name of the process set
 *
 * @param      newgroup The new group object will be returned in this argument
 *
 * @return     if !=0, an error occured
 */
int MPI_Group_from_session_pset(MPI_Session session, char const *pset_name,
                                MPI_Group *newgroup) {
  int err;
  int msg[2];
  size_t answer_size;
  MPI_Group base_group = {0};

  if (session == MPI_SESSION_NULL) {
    debug("Warning: MPI_Group_from_session_pset called with invalid session\n");
    *newgroup = MPI_GROUP_EMPTY;
    return 0;
  }
  if (pset_name == NULL) {
    debug("Warning: MPI_Group_from_session_pset called with NULL pset_name\n");
    *newgroup = MPI_GROUP_EMPTY;
    return 1;
  }

  msg[0] = session->session_id;
  msg[1] = strlen(pset_name) + 1;
  assert(msg[1] < MPI_MAX_PSET_NAME_LEN);
  err = MPI_Send(msg, 2, MPI_INT, 0, MPIDYNRES_TAG_PSET_LOOKUP,
                 g_MPIDYNRES_base_comm);
  if (err) {
    *newgroup = MPI_GROUP_EMPTY;
    return err;
  }
  err = MPI_Send(pset_name, msg[1], MPI_CHAR, 0, MPIDYNRES_TAG_PSET_LOOKUP_NAME,
                 g_MPIDYNRES_base_comm);
  if (err) {
    *newgroup = MPI_GROUP_EMPTY;
    return err;
  }
  err = MPI_Recv(&answer_size, 1, my_MPI_SIZE_T, 0,
                 MPIDYNRES_TAG_PSET_LOOKUP_ANSWER_SIZE, g_MPIDYNRES_base_comm,
                 MPI_STATUS_IGNORE);
  if (err) {
    *newgroup = MPI_GROUP_EMPTY;
    return err;
  }
  debug("Answer size: %zu\n", answer_size);

  if (answer_size == 0) {
    debug("Warning: THere was a problem getting the set\n");
    *newgroup = MPI_GROUP_EMPTY;
    return 1;
  }

  int *cr_ids = calloc(answer_size, sizeof(int));
  if (cr_ids == NULL) {
    die("Memory Error\n");
  }
  err = MPI_Recv(cr_ids, answer_size, MPI_INT, 0,
                 MPIDYNRES_TAG_PSET_LOOKUP_ANSWER, g_MPIDYNRES_base_comm,
                 MPI_STATUS_IGNORE);
  if (err) {
    return err;
  }
  for (size_t i = 0; i < answer_size; i++) {
    debug("%d\n", cr_ids[i]);
  }

  err = MPI_Comm_group(g_MPIDYNRES_base_comm, &base_group);
  if (err) {
    debug("Failed to create mpi group for base communicator\n");
    free(cr_ids);
    MPI_Group_free(&base_group);
    *newgroup = MPI_GROUP_EMPTY;
    return err;
  }
  if (base_group == MPI_GROUP_EMPTY) {
    debug("Couldn't create mpi group from base communicator\n");
    free(cr_ids);
    MPI_Group_free(&base_group);
    *newgroup = MPI_GROUP_EMPTY;
    return err;
  }

  /*BREAK();*/
  err = MPI_Group_incl(base_group, answer_size, cr_ids, newgroup);
  if (err) {
    debug("MPI_Group_incl failed\n");
    free(cr_ids);
    MPI_Group_free(&base_group);
    *newgroup = MPI_GROUP_EMPTY;
    return err;
  }

  MPI_Group_free(&base_group);
  free(cr_ids);
  return 0;
}

/**
 * @brief      Create a communicator from a group
 *
 * @param      group The MPI Group
 *
 * @param      stringtag Currently ignored
 *
 * @param      info Currently ignored
 *
 * @param      errhandler An errorhandler that will be added to the communicator
 *
 * @param      newcomm The new communicator will be returned in this argument
 *
 * @return     if != 0, an error occured
 */
int MPI_Comm_create_from_group(MPI_Group group, const char *stringtag,
                               MPI_Info info, MPI_Errhandler errhandler,
                               MPI_Comm *newcomm) {
  (void)stringtag;
  (void)info;
  int size = -1;
  int rank = -1;
  if (group == MPI_GROUP_NULL) {
    *newcomm = MPI_COMM_NULL;
    return 0;
  }
  MPI_Group_size(group, &size);
  MPI_Group_rank(group, &rank);
  debug("Creating comm from group of size %d where i have rank %d\n", size,
        rank);
  // TODO: can this tag of 0xff lead to problems?
  int err = MPI_Comm_create_group(g_MPIDYNRES_base_comm, group, 0xFF, newcomm);
  if (err) {
    debug("MPI_Comm_create_failed");
    return err;
  }
  err = MPI_Comm_set_errhandler(*newcomm, errhandler);
  if (err) {
    debug("MPI_Comm_set_errhandler_failed");
    MPI_Comm_free(newcomm);
    return err;
  }
  return 0;
}

/**
 * @brief      Create new process set by combining two existing ones
 *
 * @param      session The session used
 *
 * @param      info An info object containing hints
 *
 * @param      pset1 The first process set of the operation
 *
 * @param      pset2 The second process set of the opertaion
 *
 * @param      op The operation that should be performed
 *
 * @param      result_pset The name of the resulting process set will be
 * returned into this array, it should be at least MPI_MAX_PSET_NAME_LEN of size
 *
 * @return     if != 0, an error occured
 */
int MPIDYNRES_pset_create_op(MPI_Session session, MPI_Info hints,
                             char const pset1[], char const pset2[],
                             MPIDYNRES_pset_op op,
                             char pset_result[MPI_MAX_PSET_NAME_LEN]) {
  int err;
  MPIDYNRES_pset_op_msg msg = {0};
  msg.session_id = session->session_id;
  msg.op = op;
  strncpy(msg.pset_name1, pset1, MPI_MAX_PSET_NAME_LEN);
  strncpy(msg.pset_name2, pset2, MPI_MAX_PSET_NAME_LEN);
  err = MPI_Ssend(&msg, 1, get_pset_op_datatype(), 0, MPIDYNRES_TAG_PSET_OP,
                  g_MPIDYNRES_base_comm);
  if (err) {
    return err;
  }

  err = MPIDYNRES_Send_MPI_Info(hints, 0, MPIDYNRES_TAG_PSET_OP_INFO_SIZE,
                                MPIDYNRES_TAG_PSET_OP_INFO,
                                g_MPIDYNRES_base_comm);
  if (err) {
    return err;
  }

  err = MPI_Recv(pset_result, MPI_MAX_PSET_NAME_LEN, MPI_CHAR, 0,
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
 * @param      pset_name the name of that process set that can be freed
 *
 * @return     if != 0, an error occured
 */
int MPIDYNRES_pset_free(MPI_Session session,
                        char pset_name[MPI_MAX_PSET_NAME_LEN]) {
  int err;
  struct MPIDYNRES_pset_free_msg msg = {0};
  msg.session_id = session->session_id;
  strncpy(msg.pset_name, pset_name, MPI_MAX_PSET_NAME_LEN);
  err = MPI_Send(&msg, 1, get_pset_free_datatype(), 0, MPIDYNRES_TAG_PSET_FREE,
                 g_MPIDYNRES_base_comm);
  if (err) {
    return err;
  }
  pset_name[0] = '\0';
  return 0;
}

/**
 * @brief      Add some hints to the scheduling mechanism
 *
 * @param      session The session used
 *
 * @param      hints An info object containing hints for the scheduler
 *
 * @param      answer An info object containing an answer of the scheduler
 *
 * @return     if != 0, an error occured
 */
int MPIDYNRES_add_scheduling_hints(MPI_Session session, MPI_Info hints,
                                   MPI_Info *answer) {
  int err;
  err = MPI_Ssend(&session->session_id, 1, MPI_INT, 0,
                  MPIDYNRES_TAG_SCHED_HINTS, g_MPIDYNRES_base_comm);
  if (err) {
    return err;
  }

  err = MPIDYNRES_Send_MPI_Info(
      hints, 0, MPIDYNRES_TAG_SCHED_HINTS_SIZE,
      MPIDYNRES_TAG_SCHED_HINTS_INFO, g_MPIDYNRES_base_comm);
  if (err) {
    return err;
  }
  err = MPIDYNRES_Recv_MPI_Info(
      answer, 0, MPIDYNRES_TAG_SCHED_HINTS_ANSWER_SIZE,
      MPIDYNRES_TAG_SCHED_HINTS_ANSWER, g_MPIDYNRES_base_comm,
      MPI_STATUS_IGNORE, MPI_STATUS_IGNORE);
  if (err) {
    return err;
  }
  return 0;
}

/**
 * @brief      Ask the scheduler about resource changes
 *
 * @param      session The session used
 *
 * @param      rc_type The type of resource change is returned here
 *
 * @param      delta_pset the name of the delta set (only if no RC_NONE) is
 * returned here (result)
 *
 * @param      tag A tag that references the resource change is returned here.
 * It should be used with the MPIDYNRES_RC_accept function
 *
 * @param      info Optionally more information about the resource change is
 * returned here
 *
 * @return     if != 0, an error occured
 */
int MPIDYNRES_RC_get(MPI_Session session, MPIDYNRES_RC_type *rc_type,
                     char delta_pset[MPI_MAX_PSET_NAME_LEN],
                     MPIDYNRES_RC_tag *tag, MPI_Info *info) {
  int err;
  MPIDYNRES_RC_msg answer = {0};

  err = MPI_Send(&session->session_id, 1, MPI_INT, 0, MPIDYNRES_TAG_RC,
                 g_MPIDYNRES_base_comm);
  if (err) {
    return err;
  }
  err = MPIDYNRES_Recv_MPI_Info(info, 0, MPIDYNRES_TAG_RC_INFO_SIZE,
                                MPIDYNRES_TAG_RC_INFO, g_MPIDYNRES_base_comm,
                                MPI_STATUS_IGNORE, MPI_STATUS_IGNORE);
  if (err) {
    return err;
  }
  err = MPI_Recv(&answer, 1, get_rc_datatype(), 0, MPIDYNRES_TAG_RC_ANSWER,
                 g_MPIDYNRES_base_comm, MPI_STATUS_IGNORE);
  if (err) {
    return err;
  }
  strcpy(delta_pset, answer.pset_name);
  *rc_type = answer.type;
  *tag = answer.tag;
  return 0;
}

/**
 * @brief      Accept a resource change
 *
 * @param      session The session used
 *
 * @param      tag The tag of the resource change to accept
 *
 * @param      info Info containing hints and that will be added to session info
 * of new processes
 *
 * @return     if != 0, an error occured
 */
int MPIDYNRES_RC_accept(MPI_Session session, MPIDYNRES_RC_tag tag,
                        MPI_Info info) {
  int err;
  int msg[2];
  msg[0] = session->session_id;
  msg[1] = tag;
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
  return 0;
}

/**
 * @brief      Helper function to create info object from a string array
 *
 * @param      kvlist_size The length of the array (should be a multiple of 2)
 *
 * @param      kvlist The array of alternating key/value elements
 *
 * @param      info The new info object is returned here
 *
 * @return     if != 0, an error occured
 */
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
    assert(strlen(key) <= MPI_MAX_INFO_KEY);
    res = MPI_Info_set(*info, key, val);
    if (res) {
      *info = MPI_INFO_NULL;
      debug("Failed to set stuff in the MPI_Info object\n");
      return res;
    }
  }

  return 0;
}
