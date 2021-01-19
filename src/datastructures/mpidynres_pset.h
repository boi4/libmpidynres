#ifndef MPIDYNRES_CR_SET_H
#define MPIDYNRES_CR_SET_H

#include <stdbool.h>
#include <stddef.h>
#include <mpi.h>

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

void MPIDYNRES_print_set(MPIDYNRES_pset *set);

/*
 * MPIDYNRES_pset_create will create a new, empty MPIDYNRES_pset with internal
 * capacity cap
 */
MPIDYNRES_pset *MPIDYNRES_pset_create(size_t cap);
/*
 * MPIDYNRES_pset_destroy has to be called when done using a MPIDYNRES_pset
 * If set is NULL, the function does nothing
 */
void MPIDYNRES_pset_destroy(MPIDYNRES_pset *set);

/*
 * MPIDYNRES_pset_contains returns whether set contains the cr with id cr_id
 */
bool MPIDYNRES_pset_contains(MPIDYNRES_pset *set, int cr_id);

/*
 * MPIDYNRES_pset_from_set will create a copy of set
 */
MPIDYNRES_pset *MPIDYNRES_pset_from_set(MPIDYNRES_pset const *set);

/*
 * MPIDYNRES_pset_union will make set1 contain all crs that are part
 * of set1 or set2.
 */
void MPIDYNRES_pset_union(MPIDYNRES_pset **set1, MPIDYNRES_pset *set2);

/*
 * MPIDYNRES_pset_intersect creates make set1 contain all crs that are both
 * part of set1 or set2.
 */
void MPIDYNRES_pset_intersect(MPIDYNRES_pset *set1, MPIDYNRES_pset *set2);

/*
 * MPIDYNRES_pset_subtract removes all elements from set1 that are also part of
 * set2
 */
void MPIDYNRES_pset_subtract(MPIDYNRES_pset *set1, MPIDYNRES_pset *set2);

/*
 * MPIDYNRES_pset_add_cr adds the computing resource of cr_id to the set
 * If cr_id is already contained in the set, the function does nothing
 * Because the underlying array might be too small, a new set might be created
 * that is why the pointer might change. In that case the old set is destroyed
 * automatically
 */
void MPIDYNRES_pset_add_cr(MPIDYNRES_pset **io_set, int cr_id);

/*
 * MPIDYNRES_pset_remove_cr remove the computing resource with id cr_id from
 * the set1 if the cr_id isn't in the set already, the function does nothing
 */
void MPIDYNRES_pset_remove_cr(MPIDYNRES_pset *set, int cr_id);

/*
 * MPIDYNRES_pset_contains returns whether set contains the cr with id cr_id
 */
bool MPIDYNRES_pset_contains(MPIDYNRES_pset *set, int cr_id);

// TODO: subset

#endif
