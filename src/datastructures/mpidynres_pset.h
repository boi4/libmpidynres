#ifndef MPIDYNRES_CR_SET_H
#define MPIDYNRES_CR_SET_H

#include <stdbool.h>
#include <stddef.h>

/**
 * @brief      A Set of computing resource ids
 *
 * @details    Dynamic array
 */
struct MPIDYNRES_pset {
  size_t size;   ///< size of the set
  size_t _cap;   ///< internal capacity of array
  int cr_ids[];  ///< sorted cr ids (of length size)
};
typedef struct MPIDYNRES_pset MPIDYNRES_pset;

/*
 * MPIDYNRES_pset_destroy has to be called when done using a MPIDYNRES_pset
 * If set is NULL, the function does nothing
 */
void MPIDYNRES_pset_destroy(MPIDYNRES_pset *set);

/*
 * MPIDYNRES_pset_contains returns whether set contains the cr with id cr_id
 */
bool MPIDYNRES_pset_contains(MPIDYNRES_pset *set, int cr_id);

#endif
