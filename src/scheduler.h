/*
 * The scheduler will act as the resource manager and schedule and balance
 * resources
 */
#ifndef MPIDYNRES_SCHEDULER_H
#define MPIDYNRES_SCHEDULER_H

#include <mpi.h>
#include <stdbool.h>

struct MPIDYNRES_scheduler;
typedef struct MPIDYNRES_scheduler MPIDYNRES_scheduler;

#include "mpidynres_sim.h"
#include "scheduler_datatypes.h"
#include "scheduler_mgmt.h"

/**
 * @brief      The MPIDYNRES_scheduler struct contains information about the
 * scheduler state
 */
struct MPIDYNRES_scheduler {
  int num_scheduling_processes;  ///< number of processes available (the scheduler does not count)

  MPIDYNRES_manager *manager; ///< The manager that decides what to do when a rc request arrives

  MPIDYNRES_SIM_config *config;    ///< the scheduler config used
  set_int running_crs;  ///< the set of currently running crs, TODO: think about removing this field and just use process_states
  set_int pending_shutdowns;       ///< the set of accepted, yet not shutdown crs
  set_pset_node pset_name_map;  ///< 
  set_rc_info rc_map;  ///< 
  set_process_state process_states;

  int next_session_id; ///< the next session id to give out
  int next_rc_tag;
  bool pending_resource_change;
};
typedef struct MPIDYNRES_scheduler MPIDYNRES_scheduler;

MPIDYNRES_scheduler *MPIDYNRES_scheduler_create(MPIDYNRES_SIM_config *i_config);

void MPIDYNRES_scheduler_free(MPIDYNRES_scheduler *scheduler);

void MPIDYNRES_scheduler_start(MPIDYNRES_scheduler *scheduler);

void MPIDYNRES_scheduler_start_cr(MPIDYNRES_scheduler *scheduler, int i_cr, bool dynamic_start, int origin_rc_tag, MPI_Info origin_rc_info);

void MPIDYNRES_scheduler_shutdown_all_crs( MPIDYNRES_scheduler *scheduler);

int MPIDYNRES_scheduler_get_id_of_rank(int mpi_rank);

bool MPIDYNRES_is_reserved_pset_name(char const *pset_name);

int MPIDYNRES_gen_random_uri(char const *prefix, char res[MPI_MAX_PSET_NAME_LEN]);

#endif
