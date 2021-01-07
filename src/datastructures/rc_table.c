#include "rc_table.h"

#include <stdlib.h>
#include <string.h>

/*
 * PRIVATE
 */

static bool resize(rc_table *table, size_t new_cap) {
  if (new_cap < table->num_entries) {
    return false;
  }
  MPIDYNRES_RC_msg *new_rcs = calloc(new_cap, sizeof(MPIDYNRES_RC_msg));
  if (new_rcs == NULL) {
    return false;
  }
  for (size_t i = 0; i < table->num_entries; i++) {
    new_rcs[i] = table->rcs[i];
  }
  if (table->rcs != NULL) {
    free(table->rcs);
  }
  table->cap = new_cap;
  table->rcs = new_rcs;
  return true;
}

static int get_index(rc_table *table, int tag) {
  for (size_t i = 0; i < table->num_entries; i++) {
    if (table->rcs[i].tag == tag) {
      return i;
    }
  }
  return -1;
}

/*
 * PUBLIC
 */
rc_table *rc_table_create() {
  rc_table *res = calloc(1, sizeof(rc_table));
  if (res == NULL) {
    return NULL;
  }
  res->num_entries = 0;
  res->cap = 0;
  res->next_tag = 0;
  res->rcs = NULL;
  resize(res, 8);
  return res;
}

void rc_table_destroy(rc_table *table) {
  free(table->rcs);
  free(table);
}

int rc_table_add_entry(rc_table *table, MPIDYNRES_RC_msg rc) {
  if (get_index(table, rc.tag) == -1) {
    rc.tag = table->next_tag;
    if (table->num_entries + 1 > table->cap) {
      resize(table, table->cap * 2);
    }
    table->rcs[table->num_entries++] = rc;
    return table->next_tag++;
  }
  return -1;
}

void rc_table_rem_entry(rc_table *table, int tag) {
  int index = get_index(table, tag);
  if (index != -1) {
    memmove(&table->rcs[index], &table->rcs[index + 1],
            sizeof(MPIDYNRES_RC_msg) * (table->num_entries - index - 1));
    table->num_entries--;
  }
}

MPIDYNRES_RC_msg rc_table_get_entry(rc_table *table, int tag) {
  MPIDYNRES_RC_msg res;
  res.tag = -1;  // invalid init_info
  int index = get_index(table, tag);
  if (index != -1) {
    res = table->rcs[index];
  }
  return res;
}
