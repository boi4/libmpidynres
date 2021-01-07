#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../logging.h"
#include "mpidynres_cr_set_private.h"

/*
 * PRIVATE
 */

/**
 * @brief      Helper function to printthe contents of a set to stdout
 *
 * @param      set the set to print
 */
void MPIDYNRES_print_set(MPIDYNRES_cr_set *set) {
  printf("SET\n");
  printf("size:\t%zu\n", set->size);
  printf("cap:\t%zu\nElements:\t{ ", set->_cap);
  for (size_t i = 0; i < set->size; i++) {
    printf("%i, ", set->cr_ids[i]);
  }
  printf("}\n\n");
}

/**
 * @brief      Compare two int values
 */
static int _int_compar(void const *a, void const *b) {
  int const *a2 = (int *)a;
  int const *b2 = (int *)b;
  return (*a2 > *b2) ? 1 : (*a2 == *b2) ? 0 : -1;
}

/**
 * @brief      Remove computing resource at index index
 *
 * @param      set the cr set
 *
 * @param      index the index to remove
 */
static void MPIDYNRES_cr_set_remove_index(MPIDYNRES_cr_set *set, size_t index) {
  assert(index < set->size);
  /* MPIDYNRES_print_set(set); */
  memmove(set->cr_ids + index, set->cr_ids + (index + 1),
          (set->size - index - 1) * sizeof(int));
  set->size--;
  /* MPIDYNRES_print_set(set); */
}

/**
 * @brief      Sanitize a cr set by sorting and removing duplicates
 *
 * @param      set the set to sanitize
 */
static void MPIDYNRES_cr_set_sanitize(MPIDYNRES_cr_set *set) {
  // sort the array
  qsort(set->cr_ids, set->size, sizeof(int), _int_compar);

  // remove duplicates
  int prev = INT_MIN;
  // set->size is evaluated each round
  // TODO: check this
  for (size_t i = 0; i < set->size; i++) {
    while (i < set->size && set->cr_ids[i] == prev) {
      MPIDYNRES_cr_set_remove_index(set, i);
    }
    prev = set->cr_ids[i];
  }
}

/*
 * PUBLIC
 */

/**
 * @brief      Constructor for a new cr_set
 *
 * @param      cap the capacity of the set (only heuristical, no hard limit)
 *
 * @return     the new cr_set, NULL if there was a memory error
 */
MPIDYNRES_cr_set *MPIDYNRES_cr_set_create(size_t cap) {
  // TODO: the following multiplication can overflow and can lead to nasty
  // exploits
  assert(cap > 0);
  // TODO: check for overflow
  MPIDYNRES_cr_set *res =
      calloc(1, sizeof(struct MPIDYNRES_cr_set) + cap * sizeof(int));
  if (res == NULL) return NULL;
  res->size = 0;
  res->_cap = cap;
  return res;
}

/**
 * @brief      Copy a cr_set
 *
 * @param      set the set to copy
 *
 * @return     the new set, NULL if there was a memory error
 */
MPIDYNRES_cr_set *MPIDYNRES_cr_set_from_set(MPIDYNRES_cr_set const *set) {
  // TODO: the following multiplication can overflow and can lead to nasty
  // exploits
  MPIDYNRES_cr_set *res =
      calloc(1, sizeof(struct MPIDYNRES_cr_set) + set->_cap * sizeof(int));
  if (res == NULL) return NULL;
  res->size = set->size;
  res->_cap = set->_cap;
  memcpy(res->cr_ids, set->cr_ids, set->size * sizeof(int));
  return res;
}

/**
 * @brief      Destruct a cr_set
 *
 * @param      set the set that should be destructed
 */
void MPIDYNRES_cr_set_destroy(MPIDYNRES_cr_set *set) {
  if (set != NULL) free(set);
}

/**
 * @brief      Update a set to contain all elements in itself and another one
 *
 * @param      set1 a pointer to the set that should be updated (the set might
 * be replaced by a new set)
 *
 * @param      set2 the set that should be included in set1
 */
void MPIDYNRES_cr_set_union(MPIDYNRES_cr_set **set1, MPIDYNRES_cr_set *set2) {
  for (size_t i = 0; i < set2->size; i++) {
    MPIDYNRES_cr_set_add_cr(set1, set2->cr_ids[i]);
  }
}

/**
 * @brief      Update a set so it only contains elements that are both in the
 * set and another set
 *
 * @param      set1 the set to update
 *
 * @param      set2 the other set
 */
void MPIDYNRES_cr_set_intersect(MPIDYNRES_cr_set *set1, MPIDYNRES_cr_set *set2) {
  int *crs_to_remove = calloc(sizeof(int), set1->size);
  size_t count = 0;
  for (size_t i = 0; i < set1->size; i++) {
    if (!MPIDYNRES_cr_set_contains(set2, set1->cr_ids[i])) {
      crs_to_remove[count++] = set1->cr_ids[i];
    }
  }
  for (size_t i = 0; i < count; i++) {
    MPIDYNRES_cr_set_remove_cr(set1, crs_to_remove[i]);
  }
}

/**
 * @brief      Update a set so it only containes element that are only part of
 * it and not in the other set
 *
 * @param      set1 the set to update
 *
 * @param      set2 the other set
 */
void MPIDYNRES_cr_set_subtract(MPIDYNRES_cr_set *set1, MPIDYNRES_cr_set *set2) {
  int *crs_to_remove = calloc(sizeof(int), set1->size);
  size_t count = 0;
  for (size_t i = 0; i < set1->size; i++) {
    if (MPIDYNRES_cr_set_contains(set2, set1->cr_ids[i])) {
      crs_to_remove[count++] = set1->cr_ids[i];
    }
  }
  for (size_t i = 0; i < count; i++) {
    MPIDYNRES_cr_set_remove_cr(set1, crs_to_remove[i]);
  }
}

/**
 * @brief      Add a computing resource to a set
 *
 * @details    If the set already contains the cr_id, nothing happens
 *
 * @param      a pointer to a set that should be updated (the set might be
 * replaced by a new one)
 *
 * @param      cr_id the cr_id to be added
 */
void MPIDYNRES_cr_set_add_cr(MPIDYNRES_cr_set **set_ptr, int cr_id) {
  MPIDYNRES_cr_set *set = *set_ptr;

  if (MPIDYNRES_cr_set_contains(set, cr_id)) {
    return;
  }

  if (set->size + 1 > set->_cap) {
    // create new set with double the capacity
    size_t new_cap = 2 * set->_cap;
    *set_ptr = MPIDYNRES_cr_set_create(new_cap);
    // copy size + ids
    (*set_ptr)->size = set->size;
    memcpy(&(*set_ptr)->cr_ids, set->cr_ids, sizeof(int) * set->size);
    // work on new set from now on
    MPIDYNRES_cr_set_destroy(set);
    set = *set_ptr;
  }

  // add new element, increase size
  set->cr_ids[set->size++] = cr_id;
  // sort array
  MPIDYNRES_cr_set_sanitize(set);
}

/**
 * @brief      Remove a cr from a cr_set
 *
 * @details    if the set does not contain the element, nothing happens
 *
 * @param      set the set
 *
 * @return     return type
 */
void MPIDYNRES_cr_set_remove_cr(MPIDYNRES_cr_set *set, int cr_id) {
  int *ptr = bsearch(&cr_id, set->cr_ids, set->size, sizeof(int), _int_compar);
  if (!ptr) {
    debug(
        "Warning: tried to remove elment %d from set that doesn't contain it\n",
        cr_id);
    return;
  }
  /* MPIDYNRES_print_set(set); */
  size_t index = ptr - set->cr_ids;
  /* printf("Removing cr_id: %d at index %zu\n", cr_id, index); */
  MPIDYNRES_cr_set_remove_index(set, index);
  /* MPIDYNRES_print_set(set); */
}

/**
 * @brief      Check whether a set contains an element
 *
 * @param      set the set to check
 *
 * @param      cr_id the element to look for
 *
 * @return     whether the set contains the element
 */
bool MPIDYNRES_cr_set_contains(MPIDYNRES_cr_set *set, int cr_id) {
  for (size_t i = 0; i < set->size; i++) {
    if (set->cr_ids[i] == cr_id) {
      return true;
    }
  }
  return false;
}
