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
  MPIDYNRES_TAG_IDLE_COMMAND = 0xa00,

  MPIDYNRES_TAG_DONE_RUNNING,  // EMPTY (PLACEHOLDER INT)

  MPIDYNRES_TAG_INIT_INFO,  // EMPTY (PLACEHOLDER INT)
  MPIDYNRES_TAG_INIT_INFO_ANSWER,

  MPIDYNRES_TAG_URI_LOOKUP,  // uses MPI_CHAR
  MPIDYNRES_TAG_URI_LOOKUP_ANSWER,
  MPIDYNRES_TAG_URI_LOOKUP_ANSWER_CAP,

  MPIDYNRES_TAG_URI_SIZE,
  MPIDYNRES_TAG_URI_SIZE_ANSWER, // my_MPI_SIZE_T

  MPIDYNRES_TAG_URI_OP,
  MPIDYNRES_TAG_URI_OP_ANSWER,  // MPI_CHAR

  MPIDYNRES_TAG_URI_FREE, // MPI_CHAR

  MPIDYNRES_TAG_RC,  // EMPTY (PLACEHOLDER_INT)
  MPIDYNRES_TAG_RC_ANSWER,

  MPIDYNRES_TAG_RC_ACCEPT,
};

#define MPIDYNRES_CR_SET_INVALID SIZE_MAX

struct MPIDYNRES_idle_command {
  enum { start, shutdown } command_type;
};
typedef struct MPIDYNRES_idle_command MPIDYNRES_idle_command;

// URI OP Request
struct MPIDYNRES_URI_op_msg {
  char uri1[MPIDYNRES_URI_MAX_SIZE];
  char uri2[MPIDYNRES_URI_MAX_SIZE];
  MPIDYNRES_URI_op op;
};
typedef struct MPIDYNRES_URI_op_msg MPIDYNRES_URI_op_msg;

// Resource changes answer
struct MPIDYNRES_RC_msg {
  MPIDYNRES_RC_type type;
  int tag;
  char uri[MPIDYNRES_URI_MAX_SIZE];
};
typedef struct MPIDYNRES_RC_msg MPIDYNRES_RC_msg;

struct MPIDYNRES_RC_accept_msg {
  int rc_tag;
  int new_process_tag;
  char uri[MPIDYNRES_URI_MAX_SIZE];
};
typedef struct MPIDYNRES_RC_accept_msg MPIDYNRES_RC_accept_msg;

typedef MPIDYNRES_init_info MPIDYNRES_init_info_msg;

/*
 * This function should be called right before exiting the simulation
 */
void free_all_mpi_datatypes();

/*
 * Get MPI_Datatypes, has hidden global state
 */
MPI_Datatype get_idle_command_datatype();
MPI_Datatype get_cr_set_datatype(size_t cap);
MPI_Datatype get_uri_op_datatype();
MPI_Datatype get_rc_datatype();
MPI_Datatype get_rc_accept_datatype();
MPI_Datatype get_init_info_datatype();

#endif
