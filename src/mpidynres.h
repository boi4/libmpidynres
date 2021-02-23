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

#define MPI_MAX_PSET_NAME_LEN MPI_MAX_INFO_KEY

#define MPIDYNRES_INVALID_SESSION_ID INT_MAX

#define MPIDYNRES_NO_ORIGIN_RC_TAG -1


struct internal_mpi_session {
  int session_id;
};
typedef struct internal_mpi_session internal_mpi_session;

typedef internal_mpi_session *MPI_Session;

extern MPI_Session MPI_SESSION_INVALID;



/*
 * MPI Draft API
 */
int MPI_Session_init(MPI_Info info, MPI_Errhandler errhandler,
                     MPI_Session *session);

int MPI_Session_finalize(MPI_Session *session);

int MPI_Session_get_info(MPI_Session session, MPI_Info *info_used);

/*
 * Modified Draft API
 */
// Fill MPI_Info object with all psets that the process is part of (key: pset_name, value: pset_size)
int MPI_Session_get_psets(MPI_Session session, MPI_Info info, MPI_Info *psets);

int MPI_Session_get_pset_info(MPI_Session session, char const *pset_name,
                              MPI_Info *info);

/*
 *
 */
int MPI_Group_from_session_pset(MPI_Session session, const char *pset_name,
                                MPI_Group *newgroup);

int MPI_Comm_create_from_group(MPI_Group group, char *const stringtag, MPI_Info info, MPI_Errhandler errhandler, MPI_Comm *newcomm);

/*
 * pset Management
 */
/**
 * @brief      different set operations possible on pset
 */
enum MPIDYNRES_pset_op {
  MPIDYNRES_PSET_UNION,
  MPIDYNRES_PSET_INTERSECT,
  MPIDYNRES_PSET_DIFFERENCE,
};
typedef enum MPIDYNRES_pset_op MPIDYNRES_pset_op;

/*
 * Create a new URI based on (valid) URIs and a set operation
 */
int MPIDYNRES_pset_create_op(MPI_Session session,
                             MPI_Info input_hints,
                             char const i_pset_name1[],
                             char const i_pset_name2[],
                             MPIDYNRES_pset_op i_op,
                             char o_pset_result_name[MPI_MAX_PSET_NAME_LEN]);

/*
 * Mark pset as free, if all processes in the pset have marked it as free or have exited, it will be deleted
 */
int MPIDYNRES_pset_free(MPI_Session session, char i_pset_name[]);




/*
 * Query Runtime (Resource Manager) for Resource Changes (RCs)
 */
int MPIDYNRES_add_scheduling_hints(MPI_Session session, MPI_Info scheduling_hints, MPI_Info *answer);

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

typedef int MPIDYNRES_RC_tag;

int MPIDYNRES_RC_fetch(MPI_Session session,
                       MPIDYNRES_RC_type *o_rc_type,
                       char o_diff_pset_name[MPI_MAX_PSET_NAME_LEN],
                       MPIDYNRES_RC_tag *o_tag,
                       MPI_Info *o_info);

/*
 * Accept runtime change and provide info that will be added to the new pset
 * processes
 */
int MPIDYNRES_RC_accept(MPI_Session session, MPIDYNRES_RC_tag i_tag,
                        MPI_Info i_info);


/*
 * Exit from simulated process (real process will idle until all exit or its
 * started again)
 */
void MPIDYNRES_exit();



/*
 * Utility function to convert an array of strings (key, value alternating)
 * into an MPI_Info object
 * kvlist_size is the size of the kvlist array, NOT the number of keys (which is kvlist_size / 2)
 */
int MPIDYNRES_Info_create_strings(size_t kvlist_size, char const * const kvlist[], MPI_Info *info);

#endif
