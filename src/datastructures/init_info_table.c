#include "init_info_table.h"

#include <stdlib.h>
#include <string.h>

/*
 * PRIVATE
 */

/**
 * @brief      Try to resize a table to a new capacity
 *
 * @param      table the table to resize
 *
 * @param      new_cap the new cap
 *
 * @return     false, if there was an error
 */
static bool resize(init_info_table *table, size_t new_cap) {
  if (new_cap < table->num_entries) {
    debug("Warning: new_cap is less than number of entries\n");
    return false;
  }
  MPIDYNRES_init_info *restrict new_infos =
      calloc(new_cap, sizeof(MPIDYNRES_init_info));
  if (new_infos == NULL) {
    return false;
  }
  table->cap = new_cap;
  memcpy(new_infos, table->infos, table->num_entries * sizeof(table->infos[0]));
  if (table->infos != NULL) {
    free(table->infos);
  }
  table->infos = new_infos;
  return true;
}

/**
 * @brief      get index of init info for specific cr_id
 *
 * @param      table the table
 *
 * @param      cr_id the cr_id to look for
 *
 * @return     the index, -1 if not in ther
 */
static ssize_t get_index(init_info_table *table, int cr_id) {
  for (size_t i = 0; i < table->num_entries; i++) {
    if (table->infos[i].cr_id == cr_id) {
      return i;
    }
  }
  return -1;
}

/*
 * PUBLIC
 */
/**
 * @brief      constructor for new init_info_table
 *
 * @return     the new init_info_table or null if there was an error
 */
init_info_table *init_info_table_create() {
  init_info_table *res = calloc(1, sizeof(init_info_table));
  if (res == NULL) {
    return NULL;
  }
  res->num_entries = 0;
  res->cap = 0;
  res->infos = NULL;
  resize(res, 8);
  return res;
}

/**
 * @brief      Destructor for init_info_table
 *
 * @param      table the table to destruct
 */
void init_info_table_destroy(init_info_table *table) {
  free(table->infos);
  free(table);
}

/**
 * @brief      Add a new init_info to the table
 *
 * @details    if there is already an init_info for the cr_id in there, do
 * nothing
 *
 * @param      table the table
 *
 * @param      info the init info to add
 */
void init_info_table_add_entry(init_info_table *table, MPIDYNRES_init_info info) {
  if (get_index(table, info.cr_id) == -1) {
    if (table->num_entries + 1 > table->cap) {
      resize(table, table->cap * 2);
    }
    table->infos[table->num_entries++] = info;
  } else {
    debug(
        "Warning: Tried to add an init info although the there is already an "
        "init_info for the cr_i\n");
  }
}

/**
 * @brief      Remove the init info of a specific cr
 *
 * @details    if not in there, do nothing
 *
 * @param      table the table
 *
 * @param      cr_id the cr_id of the computing resource
 */
void init_info_table_rem_entry(init_info_table *table, int cr_id) {
  int index = get_index(table, cr_id);
  if (index != -1) {
    memmove(&table->infos[index], &table->infos[index + 1],
            sizeof(MPIDYNRES_init_info) * (table->num_entries - index - 1));
    table->num_entries--;
  } else {
    debug(
        "Warning: Tried to remove an init info of a cr_id that's not in "
        "there\n");
  }
}

/**
 * @brief      Get a saved init_info with a specific cr_id
 *
 * @param      table the table
 *
 * @param      cr_id the cr_id of the init_info to get
 *
 * @return     the init info. if info.cr_id == -1 it didn't exist in the table
 */
MPIDYNRES_init_info init_info_table_get_entry(init_info_table *table, int cr_id) {
  MPIDYNRES_init_info res;
  res.cr_id = -1;  // invalid init_info
  int index = get_index(table, cr_id);
  if (index != -1) {
    res = table->infos[index];
  } else {
    debug(
        "Warning: Tried to get an init info of a cr_id that's not in "
        "there\n");
  }
  return res;
}
