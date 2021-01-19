#include "scheduling_modes.h"

#include <tgmath.h>

#include "datastructures/mpidynres_pset.h"
#include "logging.h"

/**
 * @brief      Shuffle buffer randomly
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
static void gen_set(MPIDYNRES_scheduler *scheduler, MPIDYNRES_pset **set,
                    size_t size, bool looking_for_free_ones) {
  int perm[scheduler->num_crs];
  // generate permutations of [1...num_crs]
  gen_perm(scheduler->num_crs, perm);
  *set = MPIDYNRES_pset_create(size+1);
  for (size_t i = 0, count = 0; i < (size_t)scheduler->num_crs && count < size;
       i++) {
    if (!!looking_for_free_ones ^
        !!MPIDYNRES_pset_contains(scheduler->running_crs, perm[i])) {
      MPIDYNRES_pset_add_cr(set, perm[i]);
      count++;
    }
  }
  if ((*set)->size != size) {
    debug("GEN SET FAILED: REQUESTED MORE THAN AVAILABLE\n");
  }
}

/**
 * @brief      fill resource change object with a random difference
 *
 * @details    TODO
 *
 * @param      scheduler the scheduler
 *
 * @param      rc_msg the resource change that will be filled by this function
 */
void random_diff_scheduling(MPIDYNRES_scheduler *scheduler,
                            MPIDYNRES_RC_msg *rc_msg) {
  MPIDYNRESSIM_config *c = scheduler->config;
  MPIDYNRES_pset *set;

  int num_running = scheduler->running_crs->size;
  int num_free = scheduler->num_crs - num_running;
  debug("Num running: %d num free: %d\n", num_running, num_free);

  if ((double)rand() / (double)RAND_MAX < c->change_prob) {
    // box mueller normal approx
    double u1 = (double)rand() / (double)RAND_MAX;
    double u2 = (double)rand() / (double)RAND_MAX;
    double val = sqrt(-2.0 * log(u1)) * cos(2.0 * M_PI * u2) *
                 c->random_diff_conf.std_dev;
    int res = round(val);

    // sanitizing boundaries (not really pretty)
    if (res == 0) {
      res = val < 0 ? -1 : 1;
    }
    res = res + num_running < 0 ? -num_running + 1 : res;
    res = res > num_free ? num_free : res;

    // fill pset with random appropiate cr_ids
    if (res < 0) {
      gen_set(scheduler, &set, -res, false);
      rc_msg->type = MPIDYNRES_RC_SUB;

      // generate uri
      MPIDYNRES_uri_table_add_pset(scheduler->uri_table, set, MPI_INFO_NULL, rc_msg->uri);
      debug("Propose to remove %d crs, uri is %s\n", -res, rc_msg->uri);
      /* MPIDYNRES_print_set(scheduler->running_crs); */
      /* MPIDYNRES_print_set(set); */

      /* MPIDYNRES_pset_subtract(scheduler->running_crs, set); */  // TODO:
                                                                  // HOW TO
                                                                  // HANDLE?
      /* MPIDYNRES
      for (size_t i = 0; i < set->size; i++) {
        set_state(set->cr_ids[i], proposed_shutdown);
      }
      log_state("proposing shutown of crs");
      // TODO: think about blocking until all resources exited
    } else {
      gen_set(scheduler, &set, res, true);
      rc_msg->type = MPIDYNRES_RC_ADD;

      // generate uri
      MPIDYNRES_uri_table_add_pset(scheduler->uri_table, set, rc_msg->uri);
      debug("Propose to add %d new crs, uri is %s\n", res, rc_msg->uri);*/

      for (size_t i = 0; i < set->size; i++) {
        set_state(set->cr_ids[i], proposed);
      }
      log_state("proposing to start crs");

      MPIDYNRES_pset_union(&scheduler->running_crs, set);
    }
  } else {
    rc_msg->type = MPIDYNRES_RC_NONE;
  }
}

/**
 * @brief      fill a resource change object with by adding an increasing number
 * of crs
 *
 * @details    TODO
 *
 * @param      scheduler the scheduler
 *
 * @param      rc_msg the resource change that will be filled by this function
 */
void inc_scheduling(MPIDYNRES_scheduler *scheduler, MPIDYNRES_RC_msg *rc_msg) {
  MPIDYNRESSIM_config *c = scheduler->config;
  MPIDYNRES_pset *set;

  int num_running = scheduler->running_crs->size;
  /* printf("=================\n"); */
  /* printf("Running crs:\n"); */
  /* MPIDYNRES_print_set(scheduler->running_crs); */
  /* printf("=================\n"); */
  /* int num_free = scheduler->num_crs - num_running; */

  if ((double)rand() / (double)RAND_MAX < c->change_prob) {
    int max_running_id = scheduler->running_crs->cr_ids[num_running - 1];
    if (max_running_id != scheduler->num_crs) {
      rc_msg->type = MPIDYNRES_RC_ADD;
      set = MPIDYNRES_pset_create(8);
      MPIDYNRES_pset_add_cr(&set, max_running_id + 1);
      MPIDYNRES_uri_table_add_pset(scheduler->uri_table, set, MPI_INFO_NULL, rc_msg->uri);
      MPIDYNRES_pset_union(&scheduler->running_crs, set);
    } else {
      rc_msg->type = MPIDYNRES_RC_NONE;
    }
  } else {
    rc_msg->type = MPIDYNRES_RC_NONE;
  }
}

/**
 * @brief      fill a resource change object with by increasingly adding, then
 * increasingly subtracting crs
 *
 * @details    TODO
 *
 * @param      scheduler the scheduler
 *
 * @param      rc_msg the resource change that will be filled by this function
 */
void inc_dec_scheduling(MPIDYNRES_scheduler *scheduler, MPIDYNRES_RC_msg *rc_msg) {
  MPIDYNRESSIM_config *c = scheduler->config;
  MPIDYNRES_pset *set;
  bool kill_last;
  static int direction = 1;  // -1 is dec

  int num_running = scheduler->running_crs->size;
  /* int num_free = scheduler->num_crs - num_running; */

  double val = (double)rand() / (double)RAND_MAX;
  if (val < c->change_prob) {
    int max_running_id = scheduler->running_crs->cr_ids[num_running - 1];
    if (direction == 1 && max_running_id == scheduler->num_crs) {
      direction = -1;
    } else if (direction == -1 && max_running_id == 1) {
      if (scheduler->config->inc_dec_config.one_round_only) {
        debug("Killing last...\n");
        kill_last = true;
      }
      direction = 1;
    }
    /* debug("=========\ndirection: %d\n", direction); */
    /* MPIDYNRES_print_set(scheduler->running_crs); */
    /* debug("=========\n"); */
    if (direction == -1 || kill_last) {
      rc_msg->type = MPIDYNRES_RC_SUB;
      set = MPIDYNRES_pset_create(8);
      MPIDYNRES_pset_add_cr(&set, max_running_id);
      MPIDYNRES_uri_table_add_pset(scheduler->uri_table, set, MPI_INFO_NULL, rc_msg->uri);
      /* MPIDYNRES_pset_subtract(scheduler->running_crs, set); */
    } else {
      rc_msg->type = MPIDYNRES_RC_ADD;
      set = MPIDYNRES_pset_create(8);
      MPIDYNRES_pset_add_cr(&set, max_running_id + 1);
      MPIDYNRES_uri_table_add_pset(scheduler->uri_table, set, MPI_INFO_NULL, rc_msg->uri);
      MPIDYNRES_pset_union(&scheduler->running_crs, set);
    }
  } else {
    rc_msg->type = MPIDYNRES_RC_NONE;
  }
}
