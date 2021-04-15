#include "logging.h"

#include <errno.h>
#include <mpi.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <tgmath.h>
#include <time.h>

#include "util.h"

/**
 * Globals
 */
FILE *g_statelogfile = NULL;
enum cr_state *g_states = NULL;
size_t g_num_states = 0;
clock_t g_start_time = 0;

/**
 * State Log output colors
 */
static char *colors[NUM_CR_STATES] = {
    [idle] = BLUE,
    [running] = RED,
    [reserved] = GREEN,
    [proposed_shutdown] = GREEN,
    [accepted_shutdown] = CYAN,
};

/**
 * State Log abbreviations
 */
static int chars[NUM_CR_STATES] = {
    [idle] = 'I',
    [running] = 'R',
    [reserved] = 'P',
    [proposed_shutdown] = 'S',
    [accepted_shutdown] = 'A',
};

/**
 * @brief      Print header of states log into file
 *
 * @details    Print states header into file, with the following columns:
 * timestamp, cr state and event
 *
 * @param      f The file to print the header to
 *
 * @param      num_states The number of crs that are tracked
 */
void print_states_header(FILE *f, size_t num_states) {
  size_t max_num_digits =
      floor(log10(num_states + 1)) + 1;  // highest id is num_states + 1
  fprintf(f, "TIME       STATE  ");
  if (num_states >= 18) {
    for (size_t i = 0; i < num_states - 4; i++) {
      putc(' ', f);
    }
  }
  fprintf(f, "EVENT\n");
  for (size_t i = 0; i < num_states + 12 + 20; i++) {
    putc('-', f);
  }
  putc('\n', f);
  for (ssize_t ndigit = max_num_digits - 1; ndigit >= 0; ndigit--) {
    fprintf(f, "           ");
    for (size_t cr_id = 1; cr_id <= num_states; cr_id++) {
      size_t e = 1;
      for (ssize_t i = 0; i < ndigit; i++) e *= 10;
      int digit = (cr_id / e) % 10;
      if (cr_id >= e) {
        putc('0' + digit, f);
      } else {
        putc(' ', f);
      }
    }
    putc('\n', f);
  }
  for (size_t i = 0; i < num_states + 12 + 20; i++) {
    putc('=', f);
  }
  putc('\n', f);
}

/**
 * @brief      Print current computing resource state line into file
 *
 * @details    Print current cr state into file, different colors and characters
 * for different states. also, a small event can be printet next to the line,
 * for a better overview
 *
 * @param      f The file to write into
 *
 * @param      num_states The number of crs to track
 *
 * @param      states An array of size num_states with cr_states of the crs
 *
 * @param      eventfmt The format string for the event
 *
 * @param      args Arguments for the format string
 */
void print_states(FILE *f, size_t num_states,
                  enum cr_state states[static num_states], char const *eventfmt,
                  va_list args) {
  double diff_ms = (clock() - g_start_time) * 1000.0 / CLOCKS_PER_SEC;
  if (diff_ms < 1000000.0) {
    fprintf(f, "%8gms ", diff_ms);
  } else {
    fprintf(f, "%8gs  ", diff_ms / 1000.0);
  }
  for (size_t i = 0; i < num_states; i++) {
    fprintf(f, "%s%c%s", colors[states[i]], chars[states[i]], RESET);
  }
  putc(' ', f);
  vfprintf(f, eventfmt, args);
  putc('\n', f);
  fflush(f);
}

/**
 * @brief      Log the current state contained in the global variables
 *
 * @param      eventfmt A format string for the event
 */
void log_state(char *eventfmt, ...) {
  va_list args;
  if (getenv(STATELOG_ENVVAR)) {
    va_start(args, eventfmt);
    print_states(g_statelogfile, g_num_states, g_states, eventfmt, args);
  }
}

/**
 * @brief      Change the state of a computing resource
 *
 * @details    Change state of a specific computing resource with a specific id.
 * Uses global variables for changing state
 *
 * @param      cr_id The computing resource id of the cr to change
 *
 * @param      state The new state of the computing resource
 */
void set_state(int cr_id, enum cr_state state) {
  // little cache to improve performance
  static bool statelog_active = false;
  static bool statelog_checked = false;
  if (!statelog_checked) {
    statelog_active = (getenv(STATELOG_ENVVAR) != NULL);
    statelog_checked = true;
  }

  if (statelog_active) {
    g_states[cr_id - 1] = state;
  }
}

/**
 * @brief      Open the logfile, intialize globals and print header into file
 *
 * @param      scheduler The scheduler
 */
void init_log(MPIDYNRES_scheduler *scheduler) {
  char *logfile = getenv(STATELOG_ENVVAR);
  if (logfile) {
    g_statelogfile = fopen(logfile, "a+");
    if (g_statelogfile == NULL) {
      die("Failed to open logfile %s: %s\n", logfile, strerror(errno));
    }
    print_states_header(g_statelogfile, scheduler->num_scheduling_processes);
    g_num_states = scheduler->num_scheduling_processes;
    g_start_time = clock();
    g_states =
        calloc(sizeof(enum cr_state), scheduler->num_scheduling_processes);
    for (int i = 0; i < scheduler->num_scheduling_processes; i++) {
      g_states[i] = idle;
    }
  }
}

/**
 * @brief      Free global state used for logging
 */
void free_log() {
  if (g_states != NULL) {
    free(g_states);
    g_states = NULL;
  }
  if (g_statelogfile != NULL) {
    fclose(g_statelogfile);
    g_statelogfile = NULL;
  }
}

MPI_Comm g_debug_comm = MPI_COMM_WORLD;


/**
 * @brief      Register an MPI communicator to be used for the debug output
 * prefix
 *
 * @param      comm The MPI communicator to use (has to be valid during logging)
 */
void register_debug_comm(MPI_Comm comm) { g_debug_comm = comm; }


/**
 * @brief      Print a debug message to stderr, if the DEBUG env var is set
 *
 * @details    Different mpi ranks have different colors
 *
 * @param      fmt A format string
 */
void debug(char *fmt, ...) {
  static int myrank = -1;
  static int num_ranks = -1;
  static char prefix[0x100] = {0};
  static int prefix_set = false;
  /*static char const *const colors[] = {*/
  /*"\033[0;31m", "\033[1;31m", "\033[0;32m", "\033[1;32m",*/
  /*"\033[0;33m", "\033[01;33", "\033[0;34m", "\033[1;34m",*/
  /*"\033[0;35m", "\033[1;35m", "\033[0;36m", "\033[1;36m",*/
  /*};*/
  static char const *const colors[] = {
      "\033[0;31m", "\033[0;32m", "\033[0;33m",
      "\033[0;34m", "\033[0;35m", "\033[0;36m",
  };
  if (myrank == -1) MPI_Comm_rank(g_debug_comm, &myrank);
  if (num_ranks == -1) MPI_Comm_size(g_debug_comm, &num_ranks);
  if (!prefix_set) {
    snprintf(prefix, sizeof(prefix) - 1, "%slibmpidynres: (%d|%d) says: ",
             colors[myrank % COUNT_OF(colors)], myrank, num_ranks);
    prefix_set = true;
  }

  if (getenv(DEBUG_ENVVAR)) {
    va_list args;
    char *buf;

    va_start(args, fmt);

    buf = calloc(strlen(fmt) + strlen(prefix) + 0x100, sizeof(char));
    if (!buf) die("Memory error!");

    strcpy(buf, prefix);
    strcat(buf, fmt);
    strcat(buf, RESET);  // reset color set by prefix

    vfprintf(stderr, buf, args);
    free(buf);
  }
}
