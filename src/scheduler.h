/*
 * The scheduler will act as the resource manager and schedule and balance
 * resources
 */
#ifndef MPIDYNRES_SCHEDULER_H
#define MPIDYNRES_SCHEDULER_H

#include <mpi.h>
#include <stdbool.h>

#include "datastructures/rc_table.h"
#include "datastructures/uri_table.h"
#include "mpidynres_sim.h"

/**
 * @brief      The MPIDYNRES_scheduler struct contains information about the
 * scheduler state
 */
struct MPIDYNRES_scheduler {
  MPIDYNRESSIM_config *config;    ///< the scheduler config used
  MPIDYNRES_pset *running_crs;  ///< the set of currently running crs
  MPIDYNRES_pset *pending_shutdowns;       ///< the set of accepted, yet not shutdown crs
  MPIDYNRES_uri_table *uri_table;  ///< a lookup table which maps uris to crs
  int next_session_id; ///< the next session id to give out
  rc_table *rc_table;  ///< a lookup table which holds pending recource changes
  MPI_Info *infos; ///< array of session infos, at index n is info for cr_id n
  int num_crs;  ///< number of crs available (the scheduler does not count)
};
typedef struct MPIDYNRES_scheduler MPIDYNRES_scheduler;

MPIDYNRES_scheduler *MPIDYNRES_scheduler_create(MPIDYNRESSIM_config *i_config);

void MPIDYNRES_scheduler_destroy(MPIDYNRES_scheduler *scheduler);

void MPIDYNRES_scheduler_start(MPIDYNRES_scheduler *scheduler);

void MPIDYNRES_scheduler_start_cr(MPIDYNRES_scheduler *scheduler, int i_cr);

void MPIDYNRES_scheduler_shutdown_cr(MPIDYNRES_scheduler *scheduler, int i_cr);

void MPIDYNRES_scheduler_shutdown_all_crs( MPIDYNRES_scheduler *scheduler);

#endif
