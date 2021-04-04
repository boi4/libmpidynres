/*
 * Under normal circumstances, a library shouldn't log to stdout/stderr
 * That's why libmpidynres only logs to stdout when the env var MPIDYNRES_DEBUG is set
 * When the env var MPIDYNRES_STATELOG is set, a colourful log of the scheduler state is printed
 * to the file specified in MPIDYNRES_STATELOG
 */

#ifndef LOGGING_H
#define LOGGING_H

#include <mpi.h>
#include <stdio.h>

#include "scheduler.h"

#define RED "\033[0;31m"
#define BOLD_RED "\033[1;31m"
#define GREEN "\033[0;32m"
#define BOLD_GREEN "\033[1;32m"
#define YELLOW "\033[0;33m"
#define BOLD_YELLOW "\033[01;33m"
#define BLUE "\033[0;34m"
#define BOLD_BLUE "\033[1;34m"
#define MAGENTA "\033[0;35m"
#define BOLD_MAGENTA "\033[1;35m"
#define CYAN "\033[0;36m"
#define BOLD_CYAN "\033[1;36m"
#define RESET "\033[0m"

#define TIME_WIDTH 10

#define STATELOG_ENVVAR "MPIDYNRES_STATELOG"
#define DEBUG_ENVVAR "MPIDYNRES_DEBUG"

enum cr_state {
  idle,
  running,
  reserved,
  proposed_shutdown,
  accepted_shutdown,

  NUM_CR_STATES
};

/**
 * Globals
 */
extern FILE *g_statelogfile;
extern enum cr_state *g_states;
extern size_t g_num_states;

/**
 * Functions for State logging
 */
void log_state(char *eventfmt, ...);
void init_log(MPIDYNRES_scheduler *scheduler);
void free_log();
void set_state(int cr_id, enum cr_state state);
void print_states(FILE *f, size_t num_states,
                  enum cr_state states[static num_states], char const *eventfmt,
                  va_list args);
void print_states_header(FILE *f, size_t num_states);

/**
 * Functions for debug output
 */
void register_debug_comm(MPI_Comm comm);
void debug(char *fmt, ...);

#endif
