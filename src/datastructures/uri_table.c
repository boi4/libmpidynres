#include "uri_table.h"

#include <string.h>

#include "../mpidynres.h"
#include "../util.h"
#include "mpidynres_pset.h"

#define DEFAULT_CAP 8

/*
 * PRIVATE
 */

/**
 * @brief      Allocate + generate a random uri
 *
 * @details    detailed description
 *
 * @return     the new uri
 */
static char *MPIDYNRES_uri_gen_random_uri() {
  // TODO: check for collisions
  char const chars[] = "abcdefghijklmnopqrstuvwxyz0123456789";
  char const prefix[] = "mpi://";

  size_t n = 20;
  char *result = calloc(n + 1, sizeof(char));
  strcpy(result, prefix);
  for (size_t i = strlen(prefix); i < n; i++) {
    result[i] = chars[rand() % (COUNT_OF(chars) - 1)];  // TODO: when call srand?
  }
  return result;
}

/**
 * @brief      Lookup index of uri in table, return -1 if not in there
 *
 * @param      table the uri table
 *
 * @param      uri the uri to look up
 *
 * @return     the index in the tables array if in there, else -1
 */
static ssize_t MPIDYNRES_uri_table_get_index(MPIDYNRES_uri_table const *table,
                                   char const uri[]) {
  /* printf("TABLE: "); */
  /* for (size_t i = 0; i < table->num_entries; i++) { */
  /*   printf("\t%s\n", table->mappings[i].uri); */
  /* } */
  for (size_t i = 0; i < table->num_entries; i++) {
    if (strcmp(table->mappings[i].uri, uri) == 0) {
      return i;
    }
  }
  return -1;
}

/*
 * PUBLIC
 */

/**
 * @brief      Construct a new uri lookup table
 *
 * @return     the uri lookup table
 */
MPIDYNRES_uri_table *MPIDYNRES_uri_table_create() {
  MPIDYNRES_uri_table *result = calloc(1, sizeof(MPIDYNRES_uri_table));
  if (result == NULL) return NULL;

  result->cap = DEFAULT_CAP;
  result->num_entries = 0;
  result->mappings = calloc(result->cap, sizeof(MPIDYNRES_uri_set_pair));
  if (result->mappings == NULL) {
    free(result);
    return NULL;
  }
  return result;
}

/**
 * @brief      Destruct a uri lookup table
 *
 * @param      table the uri lookup table
 */
void MPIDYNRES_uri_table_destroy(MPIDYNRES_uri_table *table) {
  //  TODO: free from the back to get linear instead of quadratic runtime
  while (table->num_entries) {
    MPIDYNRES_uri_table_uri_free(table, table->mappings[0].uri);
  }
  free(table->mappings);
  free(table);
}

/**
 * @brief      Insert a new cr set into the lookup table, return a new uri for
 * it
 *
 * @details    The set is stored as a reference
 *
 * @param      table the uri lookup table
 *
 * @param      set the set to insert
 *
 * @param      o_uri the location where the new uri will be written to
 */
void MPIDYNRES_uri_table_add_pset(MPIDYNRES_uri_table *table, MPIDYNRES_pset *set,
                                 char o_uri[MPIDYNRES_URI_MAX_SIZE]) {
  if (table->num_entries + 1 > table->cap) {
    // TODO: overflow check
    table->mappings =
        realloc(table->mappings, sizeof(MPIDYNRES_uri_set_pair) * table->cap * 2);
    if (table->mappings == NULL) {
      die("Memory Error\n");
    }
    table->cap = 2 * table->cap;
  }
  char *uri = MPIDYNRES_uri_gen_random_uri();
  table->mappings[table->num_entries].uri = uri;
  table->mappings[table->num_entries].set = set;
  table->num_entries++;
  strcpy(o_uri, uri);
}

/**
 * @brief      Get the pset pointer associated with the uri
 *
 * @param      table the uri lookup table
 *
 * @param      uri the uri to look up
 *
 * @param      o_set the location where the set pointer is written to
 *
 * @return     whether the lookup table contains the uri
 */
bool MPIDYNRES_uri_table_lookup(MPIDYNRES_uri_table const *table, char const uri[],
                             MPIDYNRES_pset **o_set) {
  ssize_t i = MPIDYNRES_uri_table_get_index(table, uri);
  if (i == -1) return false;
  *o_set = table->mappings[i].set;
  return true;
}

/**
 * @brief      Remove uri and pset from the table
 *
 * @details    The pset is not freed by this function
 *
 * @param      table the uri lookup table
 *
 * @return     whether the lookup table contained the uri in the first place
 */
bool MPIDYNRES_uri_table_uri_free(MPIDYNRES_uri_table *table, char const uri[]) {
  // TODO: also shrink table via reallocarray
  ssize_t i = MPIDYNRES_uri_table_get_index(table, uri);
  if (i == -1) return false;
  /* MPIDYNRES_pset_destroy(table->mappings[i].set); */
  free(table->mappings[i].uri);
  memmove(table->mappings + i, table->mappings + (i + 1),
          sizeof(MPIDYNRES_uri_set_pair) * (table->num_entries - i - 1));
  table->num_entries--;
  return true;
}
