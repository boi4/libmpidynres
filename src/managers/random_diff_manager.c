#include "logging.h"
#include "math.h"
#include "scheduler_datatypes.h"
#include "scheduler_mgmt.h"
#include "util.h"

#define DEFAULT_CHANGE_PROB 1.0

struct random_diff_manager {
  MPIDYNRES_scheduler *scheduler;
  int num_processes;
  double std_dev;
  double change_prob;
};
typedef struct random_diff_manager random_diff_manager;

/**
 * @brief      Fill buf with a permutations of the numbers 1, 2, ... , size
 *
 * @details    Uses the fisher yates shuffle to generate a permutation
 * https://en.wikipedia.org/wiki/Fisher%E2%80%93Yates_shuffle
 *
 * @param      size size of buffer
 *
 * @param      buf  the buffer to shuffle
 */
void gen_perm(size_t size, int buf[size]) {
  size_t j, tmp;

  for (size_t i = 0; i < size; i++) buf[i] = i + 1;

  for (size_t i = size - 1; i + 1 > 0; i--) {
    j = rand() % (i + 1);
    tmp = buf[i];
    buf[i] = buf[j];
    buf[j] = tmp;
  }
}

/**
 * @brief      Generate a random cr set of specific size either part or not part
 * of current running crs
 *
 * @param      scheduler the scheduler
 *
 * @param      pointer where the new set will be returned
 *
 * @param      size the number of elements in the set
 *
 * @param      looking_for_free_ones whether we look for crs not in running_crs
 * or not
 */
static void gen_set(MPIDYNRES_scheduler *scheduler, set_int *set, size_t size,
                    bool looking_for_free_ones) {
  int perm[scheduler->num_scheduling_processes];
  // generate permutations of [1...num_scheduling_processes]
  gen_perm(scheduler->num_scheduling_processes, perm);
  *set = set_int_init(int_compare);
  debug("after set int init: %zu\n", set->size);
  size_t i = 0, count = 0;
  for (; i < (size_t)scheduler->num_scheduling_processes && count < size; i++) {
    if (!!looking_for_free_ones ^
        !!(set_int_count(&scheduler->running_crs, perm[i]) == 1)) {
      set_int_insert(set, perm[i]);
      count++;
    }
  }
  debug("after set insert: %zu\n", set->size);
  if (count != set->size) {
    die("Something went wrong\n");
  }
}

/*
 * No support yet
 */
int MPIDYNRES_manager_register_scheduling_hints(MPIDYNRES_manager manager,
                                                int src_process_id,
                                                MPI_Info scheduling_hints,
                                                MPI_Info *o_answer) {
  (void)manager;
  (void)src_process_id;
  (void)scheduling_hints;
  *o_answer = MPI_INFO_NULL;
  return 0;
}

MPIDYNRES_manager MPIDYNRES_manager_init(MPIDYNRES_scheduler *scheduler) {
  double sqrt_2_pi = 0.7978845608028654;  // sqrt(2/pi)
  int in_there;
  char buf[MPI_MAX_INFO_VAL];
  random_diff_manager *res;
  res = calloc(1, sizeof(random_diff_manager));
  if (res == NULL) {
    die("Memory Error\n");
  }
  res->scheduler = scheduler;
  res->num_processes = scheduler->num_scheduling_processes;

  if (scheduler->config->manager_config != NULL) {
    MPI_Info_get(scheduler->config->manager_config, "manager_std_dev",
                 MPI_MAX_INFO_VAL - 1, buf, &in_there);
  } else {
    in_there = 0;
  }
  if (in_there) {
    res->std_dev = strtod(buf, NULL);
  } else {
    res->std_dev = res->num_processes / sqrt_2_pi / 4.0;
  }

  if (scheduler->config->manager_config != NULL) {
    MPI_Info_get(scheduler->config->manager_config, "manager_change_prob",
                 MPI_MAX_INFO_VAL - 1, buf, &in_there);
  } else {
    in_there = 0;
  }
  res->change_prob = DEFAULT_CHANGE_PROB;
  if (in_there) {
    double tmp = strtod(buf, NULL);
    if (0.0 <= tmp && tmp <= 1.0) {
      res->change_prob = tmp;
    }
  }

  return res;
}

int MPIDYNRES_manager_free(MPIDYNRES_manager manager) {
  free(manager);
  return 0;
}

int MPIDYNRES_manager_get_initial_pset(MPIDYNRES_manager manager,
                                       set_int *o_initial_pset) {
  random_diff_manager *mgr = (random_diff_manager *)manager;
  int vlen;
  int in_there;
  int num_init = 1;
  MPI_Info config = mgr->scheduler->config->manager_config;

  if (config == MPI_INFO_NULL) {
    in_there = false;
  } else {
    MPI_Info_get_valuelen(config, "manager_initial_number", &vlen, &in_there);
  }
  if (in_there) {
    char *value = calloc(vlen + 1, sizeof(char));
    MPI_Info_get(config, "manager_initial_number", vlen, value, &in_there);
    num_init = atoi(value);
    free(value);
    if (num_init < 1 || num_init > mgr->num_processes) {
      die("Key manager_initial_number is invalid.\n");
    }
  } else {
    if (config == MPI_INFO_NULL) {
      in_there = false;
    } else {
      MPI_Info_get_valuelen(config, "manager_initial_number_random", &vlen,
                            &in_there);
    }
    if (in_there) {
      num_init = 1 + (rand() % (mgr->num_processes - 1));
    } else {
      num_init = 1;
    }
  }

  // choose num_init random processes
  int *perm = calloc(num_init, sizeof(int));
  if (perm == NULL) {
    die("Memory Error!");
  }
  // generate permutations of [1...num_init]
  gen_perm(num_init, perm);

  *o_initial_pset = set_int_init(int_compare);
  for (int i = 0; i < num_init; i++) {
    set_int_insert(o_initial_pset, perm[i]);
  }
  free(perm);

  return 0;
}

int MPIDYNRES_manager_handle_rc_msg(MPIDYNRES_manager manager,
                                    int src_process_id, MPI_Info *o_RC_Info,
                                    MPIDYNRES_RC_type *o_rc_type,
                                    set_int *o_new_pset) {
  (void)src_process_id;
  random_diff_manager *mgr = (random_diff_manager *)manager;
  int num_running = mgr->scheduler->running_crs.size;
  int num_free = mgr->scheduler->num_scheduling_processes - num_running;
  debug("Num running: %d num free: %d\n", num_running, num_free);

  if ((double)rand() / (double)RAND_MAX < mgr->change_prob) {
    // box mueller normal approx
    double u1 = (double)rand() / (double)RAND_MAX;
    double u2 = (double)rand() / (double)RAND_MAX;
    double val = sqrt(-2.0 * log(u1)) * cos(2.0 * M_PI * u2) * mgr->std_dev;
    int res = round(val);

    // sanitizing boundaries (not really pretty)
    if (res == 0) {
      res = val < 0 ? -1 : 1;
    }
    res = res + num_running <= 0 ? -(num_running - 1) : res;
    res = res > num_free ? num_free : res;

    debug("Res: %d\n", res);

    // fill pset with random appropiate cr_ids
    if (res < 0) {
      gen_set(mgr->scheduler, o_new_pset, -res, false);
      *o_rc_type = MPIDYNRES_RC_SUB;
      debug("Going to remove %d\n", -res);
    } else if (res > 0) {
      gen_set(mgr->scheduler, o_new_pset, res, true);
      *o_rc_type = MPIDYNRES_RC_ADD;
      debug("Going to add %d\n", res);
    } else if (res == 0) {
      *o_rc_type = MPIDYNRES_RC_NONE;
    }
  } else {
    *o_rc_type = MPIDYNRES_RC_NONE;
  }

  *o_RC_Info = MPI_INFO_NULL;

  return 0;
}
