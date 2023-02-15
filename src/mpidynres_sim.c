#include "mpidynres_sim.h"

#include <mpi.h>
#include <setjmp.h>
#include <stdbool.h>
#include <string.h>

#include "comm.h"
#include "mpidynres.h"
#include "logging.h"
#include "scheduler.h"
#include "util.h"

extern jmp_buf g_MPIDYNRES_JMP_BUF;     // defined in mpidynres.c
extern MPI_Comm g_MPIDYNRES_base_comm;  // defined in mpidynres.c

/**
 * @brief      Create, start a scheduler object (using the current process)
 *
 * @detail     This function should only be called by rank 0 as this will start
 * the scheduler
 *
 * @param      i_config       The scheduler configuration to be used
 */
static void MPIDYNRES_SIM_start_scheduler(MPIDYNRES_SIM_config *i_config) {
  MPIDYNRES_scheduler *scheduler = MPIDYNRES_scheduler_create(i_config);
  MPIDYNRES_scheduler_start(scheduler);
  MPIDYNRES_scheduler_free(scheduler);
}

/**
 * @brief      Receive an idle command via MPI
 *
 * @param      o_command The command that will be received will be written into
 * this pointer
 *
 * @param      base_comm The communicator used for communication
 * this pointer
 */
static void MPIDYNRES_SIM_get_idle_command(MPIDYNRES_idle_command *o_command,
                                MPI_Comm base_comm) {
  MPI_Datatype MPI_IDLE_COMMAND = get_idle_command_datatype();

  MPI_Recv(o_command, 1, MPI_IDLE_COMMAND, 0, MPIDYNRES_TAG_IDLE_COMMAND,
           base_comm, MPI_STATUS_IGNORE);
}

/**
 * @brief      Tell the scheduler that the cr has returned from the simulation
 * and is now idleing
 *
 * @param      base_comm The communicator used for communication
 */
static void MPIDYNRES_notify_worker_done(MPI_Comm base_comm) {
  int unused = 0;
  MPI_Ssend(&unused, 1, MPI_INT, 0, MPIDYNRES_TAG_DONE_RUNNING, base_comm);
}

/**
 * @brief      Start the simulation for a non-scheduler rank (!= 0)
 *
 * @details    Contains the main loop, that waits for commands from the
 * scheduler and then either starts or shuts down
 *
 * @param      i_config The simulation config
 *
 * @param      argc The argc argument that shall be forwarded to the simulation
 * entry
 *
 * @param      argv The argv argument that shall be forwarded to the simulation
 * entry
 *
 * @param      o_sim_main The main function that should be used as an entry
 * point for the simulation
 */
static void MPIDYNRES_SIM_start_worker(MPIDYNRES_SIM_config *i_config, int argc, char *argv[],
                            int o_sim_main(int, char **)) {
  MPIDYNRES_idle_command idle_command = {0};

  register_debug_comm(i_config->base_communicator);

  // idle loop
  bool done = false;
  while (!done) {
    // block until next command given
    MPIDYNRES_SIM_get_idle_command(&idle_command, i_config->base_communicator);

    // decide what to todo based on the command received
    switch (idle_command.command_type) {
      case shutdown: {
        debug("Got shutdown signal, quitting...\n");
        done = true;
        break;
      }
      case start: {
        debug("Got start signal, let's get rolling...\n");

        // setup return jump so simulations can call MPIDYNRES_exit()
        int val = setjmp(g_MPIDYNRES_JMP_BUF);
        if (val == 0) {
          o_sim_main(argc, argv);
        }

        debug("returned from simulation, notifying manager about it\n");
        MPIDYNRES_notify_worker_done(i_config->base_communicator);
        break;
      }
      default: {
        die("unexpected value of idle_command.command type\n");
        break;
      }
    }
  }
}

/**
 * @brief      Internal cleanup function
 */
static void cleanup() { free_all_mpi_datatypes(); }

/**
 * @brief      get a sane default config for mpidynres
 *
 * @details    uses the random diff scheduling
 *
 * @param      o_config the default config will be written to this pointer
 *
 * @return     if return value != 0, an error has happened
 */
int MPIDYNRES_SIM_get_default_config(MPIDYNRES_SIM_config *o_config) {
  o_config->base_communicator = MPI_COMM_WORLD; 
  o_config->manager_config = MPI_INFO_NULL;
  return 0;
}

/**
 * @brief      Start the mpidynres simulation
 *
 * @details    This function has to be called by all ranks inside the config
 * base communicator. it will give control to libmpidynres which handles the
 * scheduling
 *
 * @param      i_config The mpidynres config that will be used
 *
 * @param      argc The argc that should be passed to the entry function of the
 * simulated process
 *
 * @param      argv The argv that should be passed to the entry function of the
 * simulated process
 *
 * @param      i_sim_main The entry point for the simulated process
 *
 * @return     if return value != 0, an error has happened
 */
int MPIDYNRES_SIM_start(MPIDYNRES_SIM_config i_config, int argc, char *argv[],
                        int i_sim_main(int, char **)) {
  int myrank;
  int size;
  int initialized;

  // check if MPI was initialized
  MPI_Initialized(&initialized);
  if (!initialized) {
    die("You need to initialize MPI before calling MPIDYNRES_SIM_start\n");
  }

  // setup internal global variable (necessary for clean api)
  g_MPIDYNRES_base_comm = i_config.base_communicator;

  MPI_Comm_size(i_config.base_communicator, &size);
  MPI_Comm_rank(i_config.base_communicator, &myrank);

  MPI_Barrier(i_config.base_communicator);

  switch (myrank) {
    case 0: {
      debug("Am rank 0, starting scheduler\n");
      MPIDYNRES_SIM_start_scheduler(&i_config);
      break;
    }
    default: {
      debug("I'm not rank 0, waiting for commands\n");
      MPIDYNRES_SIM_start_worker(&i_config, argc, argv, i_sim_main);
      break;
    }
  }

  cleanup();

  // uncomment for debugging
  /*MPI_Errhandler_free(&eh);*/

  MPI_Barrier(i_config.base_communicator);

  return 0;
}
