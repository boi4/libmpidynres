/*
 * init_infos are stored by value
 */
#ifndef INIT_INFO_TABLE_H
#define INIT_INFO_TABLE_H

#include "../mpidynres.h"
#include "../logging.h"
#include "stddef.h"

/**
 * @brief      an init_info_table contains init_infos
 */
struct init_info_table {
  size_t num_entries; ///< the number of elements in the table
  size_t cap; ///< the internal capacity of the table
  MPIDYNRES_init_info *infos; ///< array of init_infos
};
typedef struct init_info_table init_info_table;

init_info_table *init_info_table_create();
void init_info_table_destroy(init_info_table *table);

void init_info_table_add_entry(init_info_table *table, MPIDYNRES_init_info info);
void init_info_table_rem_entry(init_info_table *table, int cr_id);
MPIDYNRES_init_info init_info_table_get_entry(init_info_table *table, int cr_id);

#endif
