/*
 * Functions that handle cr requests differently
 */
#ifndef SCHEDULING_MODES_H
#define SCHEDULING_MODES_H

#include "scheduler.h"

void random_diff_scheduling(MPIDYNRES_scheduler *scheduler, MPIDYNRES_RC_msg *rc_msg);

void inc_scheduling(MPIDYNRES_scheduler *scheduler, MPIDYNRES_RC_msg *rc_msg);

void inc_dec_scheduling(MPIDYNRES_scheduler *scheduler, MPIDYNRES_RC_msg *rc_msg);

#endif
