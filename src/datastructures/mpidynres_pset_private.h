#ifndef MPIDYNRES_CR_SET_PRIVATE_H
#define MPIDYNRES_CR_SET_PRIVATE_H

#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>

/* struct MPIDYNRES_cr_set { */
/*   size_t size;   // size of the set */
/*   size_t _cap;   // internal capacity of array */
/*   int cr_ids[];  // we will use an array here, this saves us from
 * serialization */
/* }; */
/* typedef struct MPIDYNRES_cr_set MPIDYNRES_cr_set; */

#include "mpidynres_cr_set.h"



void MPIDYNRES_print_set(MPIDYNRES_cr_set *set);

/*
 * MPIDYNRES_cr_set_create will create a new, empty MPIDYNRES_cr_set with internal
 * capacity cap
 */
MPIDYNRES_cr_set *MPIDYNRES_cr_set_create(size_t cap);

/*
 * MPIDYNRES_cr_set_from_set will create a copy of set
 */
MPIDYNRES_cr_set *MPIDYNRES_cr_set_from_set(MPIDYNRES_cr_set const *set);

/*
 * MPIDYNRES_cr_set_union will make set1 contain all crs that are part
 * of set1 or set2.
 */
void MPIDYNRES_cr_set_union(MPIDYNRES_cr_set **set1, MPIDYNRES_cr_set *set2);

/*
 * MPIDYNRES_cr_set_intersect creates make set1 contain all crs that are both
 * part of set1 or set2.
 */
void MPIDYNRES_cr_set_intersect(MPIDYNRES_cr_set *set1, MPIDYNRES_cr_set *set2);

/*
 * MPIDYNRES_cr_set_subtract removes all elements from set1 that are also part of
 * set2
 */
void MPIDYNRES_cr_set_subtract(MPIDYNRES_cr_set *set1, MPIDYNRES_cr_set *set2);

/*
 * MPIDYNRES_cr_set_add_cr adds the computing resource of cr_id to the set
 * If cr_id is already contained in the set, the function does nothing
 * Because the underlying array might be too small, a new set might be created
 * that is why the pointer might change. In that case the old set is destroyed
 * automatically
 */
void MPIDYNRES_cr_set_add_cr(MPIDYNRES_cr_set **io_set, int cr_id);

/*
 * MPIDYNRES_cr_set_remove_cr remove the computing resource with id cr_id from
 * the set1 if the cr_id isn't in the set already, the function does nothing
 */
void MPIDYNRES_cr_set_remove_cr(MPIDYNRES_cr_set *set, int cr_id);

/*
 * MPIDYNRES_cr_set_contains returns whether set contains the cr with id cr_id
 */
bool MPIDYNRES_cr_set_contains(MPIDYNRES_cr_set *set, int cr_id);

// TODO: subset

#endif
