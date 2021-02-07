/*
 * Structs + MPI_Datatypes for communicating with the resource manager
 */
#ifndef COMM_H
#define COMM_H

#include <assert.h>
#include <mpi.h>

#include "mpidynres.h"
#include "util.h"

/*
 * TAG definitions (code the message type/content)
 */
enum {
  MPIDYNRES_TAG_IDLE_COMMAND = 0xa000,

  MPIDYNRES_TAG_DONE_RUNNING,  // EMPTY (PLACEHOLDER INT)

  MPIDYNRES_TAG_SESSION_CREATE,  // placeholder
  MPIDYNRES_TAG_SESSION_CREATE_ANSWER,  // session_tag

  MPIDYNRES_TAG_SESSION_INFO,         // session_tag
  MPIDYNRES_TAG_SESSION_INFO_ANSWER_SIZE,
  MPIDYNRES_TAG_SESSION_INFO_ANSWER,

  MPIDYNRES_TAG_SESSION_FINALIZE,         // session_tag
  MPIDYNRES_TAG_SESSION_FINALIZE_ANSWER,  // int (ok or problem)

  MPIDYNRES_TAG_GET_PSETS,         // session_tag
  MPIDYNRES_TAG_GET_PSETS_INFO_SIZE,
  MPIDYNRES_TAG_GET_PSETS_INFO,
  MPIDYNRES_TAG_GET_PSETS_ANSWER_SIZE,
  MPIDYNRES_TAG_GET_PSETS_ANSWER,

  MPIDYNRES_TAG_PSET_INFO,         // session_id + size of name
  MPIDYNRES_TAG_PSET_INFO_NAME,         // name
  MPIDYNRES_TAG_PSET_INFO_ANSWER_SIZE,
  MPIDYNRES_TAG_PSET_INFO_ANSWER,

  MPIDYNRES_TAG_PSET_LOOKUP,  // session_id +sizeof name
  MPIDYNRES_TAG_PSET_LOOKUP_NAME,
  MPIDYNRES_TAG_PSET_LOOKUP_ANSWER_CAP,
  MPIDYNRES_TAG_PSET_LOOKUP_ANSWER,

  MPIDYNRES_TAG_PSET_OP,
  MPIDYNRES_TAG_PSET_OP_ANSWER,  // MPI_CHAR

  MPIDYNRES_TAG_PSET_FREE,  // MPI_CHAR

  MPIDYNRES_TAG_RC,  // EMPTY (PLACEHOLDER_INT)
  MPIDYNRES_TAG_RC_ANSWER,

  MPIDYNRES_TAG_RC_ACCEPT,
  MPIDYNRES_TAG_RC_ACCEPT_INFO_SIZE,
  MPIDYNRES_TAG_RC_ACCEPT_INFO,
  MPIDYNRES_TAG_RC_ACCEPT_ANSWER,
};

#define MPIDYNRES_CR_SET_INVALID SIZE_MAX

struct MPIDYNRES_idle_command {
  enum { start, shutdown } command_type;
};
typedef struct MPIDYNRES_idle_command MPIDYNRES_idle_command;

// pset OP Request
struct MPIDYNRES_pset_op_msg {
  int session_id;
  char uri1[MPI_MAX_PSET_NAME_LEN];
  char uri2[MPI_MAX_PSET_NAME_LEN];
  MPIDYNRES_pset_op op;
};
typedef struct MPIDYNRES_pset_op_msg MPIDYNRES_pset_op_msg;

// Resource changes answer
// TODO
struct MPIDYNRES_RC_msg {
  MPIDYNRES_RC_type type;
  int tag;
  char uri[MPI_MAX_PSET_NAME_LEN];
};
typedef struct MPIDYNRES_RC_msg MPIDYNRES_RC_msg;

// TODO
struct MPIDYNRES_RC_accept_msg {
  int rc_tag;
  int new_process_tag;
  char uri[MPI_MAX_PSET_NAME_LEN];
};
typedef struct MPIDYNRES_RC_accept_msg MPIDYNRES_RC_accept_msg;

struct MPIDYNRES_pset_free_msg {
  int session_id;
  char pset_name[MPI_MAX_PSET_NAME_LEN];
};
typedef struct MPIDYNRES_pset_free_msg MPIDYNRES_pset_free_msg;

/*
 * This function should be called right before exiting the simulation
 */
void free_all_mpi_datatypes();

/*
 * Serialization of an MPI_Info object
 */
struct info_serialized {
  size_t num_strings;   // number of strings (has to be multiple of 2)
  ptrdiff_t strings[];  // difference from the base of this struct
};

int MPIDYNRES_Send_MPI_Info(MPI_Info info, int dest, int tag1, int tag2,
                            MPI_Comm comm);
int MPIDYNRES_Recv_MPI_Info(MPI_Info *info, int source, int tag1, int tag2,
                            MPI_Comm comm, MPI_Status *status1,
                            MPI_Status *status2);

/*
 * Get MPI_Datatypes, has hidden global state
 */
MPI_Datatype get_idle_command_datatype();
MPI_Datatype get_pset_datatype(size_t cap);
MPI_Datatype get_pset_op_datatype();
MPI_Datatype get_pset_free_datatype();
MPI_Datatype get_rc_datatype();
#endif
