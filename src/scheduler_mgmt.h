#ifndef SCHEDULER_MGMT_H
#define SCHEUDLER_MGMT_H
/*
 * This header defines a general interface that can be used to query a
 * scheduling management impelmentation for scheduling decisions
 */

/*
 * The MPIDYNRES_manager decides on how to act when cr ids ask for resource
 * changes To allow different management implementations, the manager is
 * typedef'd to a void *
 */
typedef void *MPIDYNRES_manager;

#include "scheduler.h"

MPIDYNRES_manager MPIDYNRES_manager_init(MPIDYNRES_scheduler *scheduler);

int MPIDYNRES_manager_free(MPIDYNRES_manager manager);

/*
 * scheduling_hints has to be copied, as it will be freed afterwards
 */
int MPIDYNRES_manager_register_scheduling_hints(MPIDYNRES_manager manager,
                                                int src_process_id,
                                                MPI_Info scheduling_hints,
                                                MPI_Info *o_answer);

int MPIDYNRES_manager_get_initial_pset(MPIDYNRES_manager manager,
                                       set_int *o_initial_pset);

int MPIDYNRES_manager_handle_rc_msg(MPIDYNRES_manager manager,
                                    int src_process_id, MPI_Info *o_RC_Info,
                                    MPIDYNRES_RC_type *o_rc_type,
                                    set_int *o_new_pset);

#endif
