/*
 * URI Lookup table, very far away from being efficient
 * URIs are saved as copies
 * CR_Sets are saved by reference !!
 */
#ifndef MPIDYNRES_URI_TABLE_H
#define MPIDYNRES_URI_TABLE_H

#include "../mpidynres.h"  // MPI_MAX_PSET_NAME_LEN
#include "mpidynres_pset.h"

struct MPIDYNRES_uri_set_pair {
  char *uri;
  MPIDYNRES_pset *set;
  MPI_Info pset_info;
};
typedef struct MPIDYNRES_uri_set_pair MPIDYNRES_uri_set_pair;

/**
 * @brief      A MPIDYNRES_uri_table holds uri->pset mappings ((the uri will be
 * copied, the set is hold by reference only, but will be freed on destruction))
 */
struct MPIDYNRES_uri_table {
  size_t num_entries;                ///< number of elements
  size_t cap;                        ///< internal capacity
  MPIDYNRES_uri_set_pair *mappings;  ///< the elements
};
typedef struct MPIDYNRES_uri_table MPIDYNRES_uri_table;

/*
 * Constructor
 */
MPIDYNRES_uri_table *MPIDYNRES_uri_table_create();

/*
 * Destructor
 */
void MPIDYNRES_uri_table_destroy(MPIDYNRES_uri_table *table);

/*
 * Insert set and return uri that will be used to associate the set
 */
void MPIDYNRES_uri_table_add_pset(MPIDYNRES_uri_table *table,
                                  MPIDYNRES_pset *i_set, MPI_Info info,
                                  char o_uri[MPI_MAX_PSET_NAME_LEN]);
/*
 * Lookup uri and save cr in o_set, false if not in there
 */
bool MPIDYNRES_uri_table_lookup(MPIDYNRES_uri_table const *table,
                                char const i_uri[], MPIDYNRES_pset **o_set,
                                MPI_Info *o_info);

/* /\* */
/*  * Remove uri+pset from table, false if not in there */
/*  *\/ */
/* bool MPIDYNRES_uri_table_uri_free(MPIDYNRES_uri_table *table, */
/*                                   char const i_uri[]); */

#endif
