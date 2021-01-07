#ifndef MPIDYNRES_CR_SET_H
#define MPIDYNRES_CR_SET_H

#include <stdbool.h>
#include <stddef.h>

/**
 * @brief      A Set of computing resource ids
 *
 * @details    Dynamic array
 */
struct MPIDYNRES_cr_set {
  size_t size;   ///< size of the set
  size_t _cap;   ///< internal capacity of array
  int cr_ids[];  ///< sorted cr ids (of length size)
};
typedef struct MPIDYNRES_cr_set MPIDYNRES_cr_set;

/*
 * MPIDYNRES_cr_set_destroy has to be called when done using a MPIDYNRES_cr_set
 * If set is NULL, the function does nothing
 */
void MPIDYNRES_cr_set_destroy(MPIDYNRES_cr_set *set);

/*
 * MPIDYNRES_cr_set_contains returns whether set contains the cr with id cr_id
 */
bool MPIDYNRES_cr_set_contains(MPIDYNRES_cr_set *set, int cr_id);

#endif
