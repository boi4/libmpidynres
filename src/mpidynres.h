/*
 * Functions defined in this file can be called from the simulated application
 * They are used for interaction with the simulated runtime (the resource
 * manager)
 */
#ifndef MPIDYNRES_H
#define MPIDYNRES_H

#include <limits.h>
#include <mpi.h>
#include <stdbool.h>
#include <stddef.h>

#include "mpidynres_pset.h"

#define MPIDYNRES_URI_MAX_SIZE 1024  // including '\0'

#define MPIDYNRES_TAG_FIRST_START INT_MAX

/**
 * @brief      information about the simulated environemnt
 *
 * @details    can be requested by calling MPIDYNRES_init_info_get
 */
struct MPIDYNRES_init_info {
  char uri_init[MPIDYNRES_URI_MAX_SIZE];    ///< the uri of newly created set
  char uri_passed[MPIDYNRES_URI_MAX_SIZE];  ///< uri passed by "rc accept"
  int init_tag;  ///< tag passed by "rc accept" or MPIDYNRES_TAG_STARTUP
  int cr_id;     ///< the cr_id of the current process
};
typedef struct MPIDYNRES_init_info MPIDYNRES_init_info;

/*
 * Get init_info for current simulated process
 */
int MPIDYNRES_init_info_get(MPIDYNRES_init_info *o_init_info);

/*
 * Exit from simulated process (real process will idle until all exit or its
 * started again)
 */
void MPIDYNRES_exit();

/*
 * Get CR_Set associated with uri
 */
int MPIDYNRES_URI_lookup(char const i_uri[], MPIDYNRES_pset **o_set);

/*
 * Get size of CR_Set associated with uri
 */
int MPIDYNRES_URI_size(char const i_uri[], size_t *o_size);

/**
 * @brief      different set operations possible on uris
 */
enum MPIDYNRES_URI_op {
  MPIDYNRES_URI_UNION,
  MPIDYNRES_URI_INTERSECT,
  MPIDYNRES_URI_SUBTRACT,
};
typedef enum MPIDYNRES_URI_op MPIDYNRES_URI_op;

/*
 * Create a new URI based on (valid) URIs and a set operation
 */
int MPIDYNRES_URI_create_op(char const i_uri1[], char const i_uri2[],
                         MPIDYNRES_URI_op i_op,
                         char o_uri_result[MPIDYNRES_URI_MAX_SIZE]);

/*
 * Free uri. The URI and communicators based on the uri are invalid.
 * Also sets io_uri to nullbytes
 */
int MPIDYNRES_URI_free(char io_uri[]);

/*
 * Create a communicator from an URI. Blocking. Every cr in the set specified by
 * uri has to call this function. It will block until all sets have called it
 */
int MPIDYNRES_Comm_create_uri(char const i_uri[MPIDYNRES_URI_MAX_SIZE],
                           MPI_Comm *o_comm);

/*
 * Query Runtime (Resource Manager) for Resource Changes (RCs)
 * In case of MPIDYNRES_RC_SUB
 */
/**
 * @brief      Different types of resource changes
 */
enum MPIDYNRES_RC_type {
  MPIDYNRES_RC_NONE,
  MPIDYNRES_RC_ADD,
  MPIDYNRES_RC_SUB,
  MPIDYNRES_RC_REPLACE,
};
typedef enum MPIDYNRES_RC_type MPIDYNRES_RC_type;

int MPIDYNRES_RC_fetch(MPIDYNRES_RC_type *o_rc_type,
                    char o_diff_uri[MPIDYNRES_URI_MAX_SIZE], int *o_tag);

/*
 * Accept runtime change and provide uri that will be passed to all new
 * processes
 */
int MPIDYNRES_RC_accept(int i_rc_tag, int i_new_process_tag,
                     char i_uri_msg[MPIDYNRES_URI_MAX_SIZE]);

// TODO: add a uri contains me function
#endif
