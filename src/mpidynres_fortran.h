#pragma once

#include "mpidynres_sim.h"


/*
** TODO:
**    - add ierror instead of int return
**    - free mpi conversion data
**    - link mpidynres.f90 and mpidynres_sim.f90 into libmpidynres.so
**    - all mpidynres functions that return string, need to first set it to zero!
*/


/**
 * @brief      This struct is the fortran equivalent of MPIDYNRES_SIM_config
 * scheduling
 */
struct FMPIDYNRES_SIM_config {
  /*
   * The base_communicator contains all computing resources available for the
   * simulation
   */
  MPI_Fint base_communicator;
  MPI_Fint manager_config;
};
typedef struct FMPIDYNRES_SIM_config FMPIDYNRES_SIM_config;



int FMPIDYNRES_SIM_get_default_config(FMPIDYNRES_SIM_config *o_fconfig);


int FMPIDYNRES_SIM_start(FMPIDYNRES_SIM_config *i_config, int i_sim_main(int, char**));




/*
 * MPI Draft API
 */
int FMPI_Session_init(MPI_Fint *info, MPI_Fint *errhandler,
                      MPI_Session *session);

int FMPI_Session_finalize(MPI_Session *session);
int FMPI_Session_get_info(MPI_Session *session, MPI_Fint *info_used);

/*
 * Modified Draft API
 */
int FMPI_Session_get_psets(MPI_Session *session, MPI_Fint *info, MPI_Fint *psets);

int FMPI_Session_get_pset_info(MPI_Session *session, char const *pset_name,
                              MPI_Fint *info);

int FMPI_Group_from_session_pset(MPI_Session *session, const char *pset_name,
                                MPI_Fint *newgroup);

int FMPI_Comm_create_from_group(MPI_Fint *group, const char *stringtag,
                                MPI_Fint *info, MPI_Fint *errhandler, MPI_Fint *newcomm);

int FMPIDYNRES_pset_create_op(MPI_Session *session,
                             MPI_Fint *hints,
                             char const pset1[],
                             char const pset2[],
                             MPIDYNRES_pset_op *op,
                             char pset_result[MPI_MAX_PSET_NAME_LEN]);

int FMPIDYNRES_pset_free(MPI_Session *session, char i_pset_name[MPI_MAX_PSET_NAME_LEN]);




int FMPIDYNRES_add_scheduling_hints(MPI_Session *session, MPI_Fint *hints, MPI_Fint *answer);

int FMPIDYNRES_RC_get(MPI_Session *session,
                     MPIDYNRES_RC_type *rc_type,
                     char delta_pset[MPI_MAX_PSET_NAME_LEN],
                     MPIDYNRES_RC_tag *tag,
                      MPI_Fint *info);

int FMPIDYNRES_RC_accept(MPI_Session *session, MPIDYNRES_RC_tag *tag,
                        MPI_Fint *info);


/*
 * Exit from simulated process (real process will idle until all exit or its
 * started again)
 */
void FMPIDYNRES_exit();



/* /\* */
/*  * Utility function to convert an array of strings (key, value alternating) */
/*  * into an MPI_Info object */
/*  * kvlist_size is the size of the kvlist array, NOT the number of keys (which is kvlist_size / 2) */
/*  *\/ */
/* int FMPIDYNRES_Info_create_strings(size_t kvlist_size, char const * const kvlist[], MPI_Info *info); */
