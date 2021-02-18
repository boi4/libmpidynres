/*
 * The scheduler will act as the resource manager and schedule and balance
 * resources
 */
#ifndef MPIDYNRES_SCHEDULER_H
#define MPIDYNRES_SCHEDULER_H

#include <mpi.h>
#include <stdbool.h>

#include "mpidynres_sim.h"
#include "scheduler_datatypes.h"

/**
 * @brief      The MPIDYNRES_scheduler struct contains information about the
 * scheduler state
 */
struct MPIDYNRES_scheduler {
  int num_scheduling_processes;  ///< number of processes available (the scheduler does not count)

  MPIDYNRESSIM_config *config;    ///< the scheduler config used
  set_int running_crs;  ///< the set of currently running crs
  set_int pending_shutdowns;       ///< the set of accepted, yet not shutdown crs
  set_pset_node pset_name_map;  ///< 
  set_rc_node rc_map;  ///< 
  set_process_state process_states;

  int next_session_id; ///< the next session id to give out
  int next_rc_tag;
  bool pending_resource_change;
};
typedef struct MPIDYNRES_scheduler MPIDYNRES_scheduler;

MPIDYNRES_scheduler *MPIDYNRES_scheduler_create(MPIDYNRESSIM_config *i_config);

void MPIDYNRES_scheduler_destroy(MPIDYNRES_scheduler *scheduler);

void MPIDYNRES_scheduler_start(MPIDYNRES_scheduler *scheduler);

void MPIDYNRES_scheduler_start_cr(MPIDYNRES_scheduler *scheduler, int i_cr, bool dynamic_start, int origin_rc_tag);

void MPIDYNRES_scheduler_shutdown_all_crs( MPIDYNRES_scheduler *scheduler);

int MPIDYNRES_scheduler_get_id_of_rank(int mpi_rank);

#endif
