/*
 * Implementation of the functions called by the simulated processes
 */
#include "mpidynres.h"

#include <mpi.h>
#include <setjmp.h>
#include <stdint.h>
#include <string.h>

#include "comm.h"
#include "datastructures/mpidynres_cr_set_private.h"
#include "logging.h"
#include "util.h"

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
 * @brief      Get init_info for current cr from scheduler
 *
 * @param      o_init_info the pointer where the init_info will be written to
 *
 * @return     if != 0, an error has happened
 */
int MPIDYNRES_init_info_get(MPIDYNRES_init_info *o_init_info) {
  int unused = 0;
  MPI_Send(&unused, 1, MPI_INT, 0, MPIDYNRES_TAG_INIT_INFO, g_MPIDYNRES_base_comm);
  MPI_Recv(o_init_info, 1, get_init_info_datatype(), 0,
           MPIDYNRES_TAG_INIT_INFO_ANSWER, g_MPIDYNRES_base_comm, MPI_STATUS_IGNORE);
  return 0;
}

/**
 * @brief      Get cr set associated with URI from the scheduler
 *
 * @param      i_uri the uri that should be looked up
 *
 * @param      o_set the pointer where the new cr set will be written to. It has
 * to be deconstructed by calling MPIDYNRES_cr_set_destroy
 *
 * @return     if != 0, an error has happened
 */
int MPIDYNRES_URI_lookup(char const i_uri[], MPIDYNRES_cr_set **o_set) {
  MPI_Datatype t;
  MPIDYNRES_cr_set *tmp_set;
  size_t cap;

  // send uri
  assert(strlen(i_uri) < MPIDYNRES_URI_MAX_SIZE);
  MPI_Send(i_uri, strlen(i_uri) + 1, MPI_CHAR, 0, MPIDYNRES_TAG_URI_LOOKUP,
           g_MPIDYNRES_base_comm);

  // first receive cap
  MPI_Recv(&cap, 1, my_MPI_SIZE_T, 0, MPIDYNRES_TAG_URI_LOOKUP_ANSWER_CAP,
           g_MPIDYNRES_base_comm, MPI_STATUS_IGNORE);

  if (cap == MPIDYNRES_CR_SET_INVALID) {
    *o_set = NULL;
    return 1;
  }
  debug("want to receive cr_set. Capacity is : %zu\n", cap);

  // allocate cr_set
  tmp_set = MPIDYNRES_cr_set_create(cap);

  // then receive cr_set
  t = get_cr_set_datatype(cap);
  MPI_Recv(tmp_set, 1, t, 0, MPIDYNRES_TAG_URI_LOOKUP_ANSWER, g_MPIDYNRES_base_comm,
           MPI_STATUS_IGNORE);
  MPI_Type_free(&t);

  *o_set = tmp_set;
  return 0;
}

/**
 * @brief      ask scheduler for size of a cr set referenced by uri
 *
 * @param      i_uri the uri whose size should be lookup up
 *
 * @param      o_size the pointer where the size will be written to
 *
 * @return     if != 0, an error has happened
 */
int MPIDYNRES_URI_size(char const i_uri[], size_t *o_size) {
  assert(strlen(i_uri) < MPIDYNRES_URI_MAX_SIZE);
  MPI_Send(i_uri, strlen(i_uri) + 1, MPI_CHAR, 0, MPIDYNRES_TAG_URI_SIZE,
           g_MPIDYNRES_base_comm);
  MPI_Recv(o_size, 1, my_MPI_SIZE_T, 0, MPIDYNRES_TAG_URI_SIZE_ANSWER,
           g_MPIDYNRES_base_comm, MPI_STATUS_IGNORE);
  return 0;
}

/**
 * @brief      ask the scheduler for a new uri that contains the contents of a
 * set operation applied on two cr sets
 *
 * @param      i_uri1 the first uri of the operation
 *
 * @param      i_uri2 the second uri of the opertaion
 *
 * @param      i_op the operation that should be performed
 *
 * @param      o_uri_result the array where the resulting uri should be written
 * to
 *
 * @return     if != 0, an error has happened
 */
int MPIDYNRES_URI_create_op(char const i_uri1[], char const i_uri2[],
                         MPIDYNRES_URI_op i_op,
                         char o_uri_result[MPIDYNRES_URI_MAX_SIZE]) {
  MPIDYNRES_URI_op_msg msg = {0};
  msg.op = i_op;
  strncpy(msg.uri1, i_uri1, MPIDYNRES_URI_MAX_SIZE - 1);
  strncpy(msg.uri2, i_uri2, MPIDYNRES_URI_MAX_SIZE - 1);
  MPI_Send(&msg, 1, get_uri_op_datatype(), 0, MPIDYNRES_TAG_URI_OP,
           g_MPIDYNRES_base_comm);
  MPI_Recv(o_uri_result, MPIDYNRES_URI_MAX_SIZE, MPI_CHAR, 0,
           MPIDYNRES_TAG_URI_OP_ANSWER, g_MPIDYNRES_base_comm, MPI_STATUS_IGNORE);
  return 0;
}

/**
 * @brief      ask the scheduler about resource changes
 *
 * @param      o_rc_type the rc_type (result)
 *
 * @param      o_diff_uri the uri of the delta set (only if no RC_NONE) (result)
 *
 * @param      o_tag the tag/handle for the reosurce change that can be used
 * with RC_accept
 *
 * @return     if != 0, an error has happened
 */
int MPIDYNRES_RC_fetch(MPIDYNRES_RC_type *o_rc_type,
                    char o_diff_uri[MPIDYNRES_URI_MAX_SIZE], int *o_tag) {
  int unused = 0;
  MPIDYNRES_RC_msg answer;

  MPI_Ssend(&unused, 1, MPI_INT, 0, MPIDYNRES_TAG_RC, g_MPIDYNRES_base_comm);
  MPI_Recv(&answer, 1, get_rc_datatype(), 0, MPIDYNRES_TAG_RC_ANSWER,
           g_MPIDYNRES_base_comm, MPI_STATUS_IGNORE);
  strcpy(o_diff_uri, answer.uri);
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
int MPIDYNRES_RC_accept(int i_rc_tag, int i_new_process_tag,
                     char i_uri_msg[MPIDYNRES_URI_MAX_SIZE]) {
  MPIDYNRES_RC_accept_msg msg = {0};
  msg.rc_tag = i_rc_tag;
  msg.new_process_tag = i_new_process_tag;
  strncpy(msg.uri, i_uri_msg, MPIDYNRES_URI_MAX_SIZE - 1);
  MPI_Send(&msg, 1, get_rc_accept_datatype(), 0, MPIDYNRES_TAG_RC_ACCEPT,
           g_MPIDYNRES_base_comm);

  return 0;
}

/**
 * @brief      tell the resource manager that an uri can be freed
 *
 * @param      io_uri the uri that can be freed
 *
 * @return     if != 0, an error has happened
 */
int MPIDYNRES_URI_free(char io_uri[MPIDYNRES_URI_MAX_SIZE]) {
  MPI_Send(&io_uri, MPIDYNRES_URI_MAX_SIZE, MPI_CHAR, 0, MPIDYNRES_TAG_URI_FREE,
           g_MPIDYNRES_base_comm);
  memset(io_uri, 0, MPIDYNRES_URI_MAX_SIZE * sizeof(char));
  return 0;
}

/**
 * @brief      create a new MPI communicator with all crs contained in uri
 *
 * @details    this function has to be called by all members of the cr set
 * contained in uri
 *
 * @param      i_uri the uri that contains the set that the communicator will be
 * based on
 *
 * @param      o_comm the new communicator will be written to this pointer
 *
 * @return     if != 0, an error has happened
 */
int MPIDYNRES_Comm_create_uri(char const i_uri[MPIDYNRES_URI_MAX_SIZE],
                           MPI_Comm *o_comm) {
  static MPI_Group base_group = {0};
  static bool base_group_set = false;

  MPIDYNRES_cr_set *cr_set = NULL;
  MPI_Group set_group = {0};
  int res, err;

  if (!base_group_set) {
    debug("Creating group from base communicator\n");
    res = MPI_Comm_group(g_MPIDYNRES_base_comm, &base_group);
    if (res) {
      die("Failed to create mpi group for base communicator\n");
    }
    if (base_group == MPI_GROUP_EMPTY) {
      die("Couldn't create mpi group from base communicator\n");
    }
    base_group_set = true;
  }

  MPIDYNRES_URI_lookup(i_uri, &cr_set);
  if (cr_set == NULL) {
    return -1;
  }
  err =
      MPI_Group_incl(base_group, (int)cr_set->size, cr_set->cr_ids, &set_group);
  /* MPIDYNRES_cr_set_destroy(cr_set); */
  if (err) {
    die("MPI_Group_incl failed\n");
  }
  if (set_group == MPI_GROUP_EMPTY) {
    die("MPI_Group_incl failed\n");
  }
  err = MPI_Comm_create_group(g_MPIDYNRES_base_comm, set_group, 0, o_comm);
  if (err) {
    die("MPI_Comm_create_group failed\n");
  }
  if (*o_comm == MPI_COMM_NULL) {
    die("NULL communicator was created\n");
  }
  return 0;
}
