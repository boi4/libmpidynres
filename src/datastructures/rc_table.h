/*
 * rc msgs are stored by value
 */
#ifndef RC_TABLE_H
#define RC_TABLE_H

#include "../comm.h"
#include "../mpidynres.h"

#include "stddef.h"

struct rc_table {
  size_t num_entries;
  size_t cap;
  int next_tag;
  MPIDYNRES_RC_msg *rcs;
};
typedef struct rc_table rc_table;


rc_table *rc_table_create();
void rc_table_destroy(rc_table *table);

// return and set tag
int rc_table_add_entry(rc_table *table, MPIDYNRES_RC_msg rc);

// don't forget to remove url from the url table
void rc_table_rem_entry(rc_table *table, int tag);

MPIDYNRES_RC_msg rc_table_get_entry(rc_table *table, int tag);

#endif
