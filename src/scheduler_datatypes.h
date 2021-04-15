#ifndef SCHEDULER_DATATYPES_H
#define SCHEDULER_DATATYPES_H
/*
 * The datastructures used by the scheduler are part of the ctl project (see
 * 3rdpart/ctl) Currently, the scheduler calls the functions provided by ctl
 * directly This could be changed in the future by writing wrapper
 * structures/functions which would allow for more flexibility in the setup (but
 * also leads to some boilerplate)
 */

#include <stdbool.h>

#include "comm.h"
#include "mpidynres.h"

// set_int
#define P
#define T int
#include <set.h>
int int_compare(int *a, int *b);

// set_pset_node
struct pset_node {
  char pset_name[MPI_MAX_PSET_NAME_LEN];
  set_int pset;
  MPI_Info pset_info;  // should include how the pset was created and arguments
};
typedef struct pset_node pset_node;
void pset_node_free(pset_node *pn);
pset_node pset_node_copy(pset_node *pn);
int pset_node_compare(pset_node *a, pset_node *b);
#undef P
#define T pset_node
#include <set.h>

int set_pset_node_find_by_name(set_pset_node *set, char const *name,
                               pset_node **res);

// set_rc_info
struct rc_info {
  int rc_tag;
  char new_pset_name[MPI_MAX_PSET_NAME_LEN];
  set_int pset;  // copy if real one is deleted
  MPIDYNRES_RC_type rc_type;
};
typedef struct rc_info rc_info;
void rc_info_free(rc_info *rn);
rc_info rc_info_copy(rc_info *rn);
int rc_info_compare(rc_info *a, rc_info *b);
#undef P
#define T rc_info
#include <set.h>
int set_rc_info_find_by_tag(set_rc_info *set, int tag, rc_info **res);

// set_pset_name
struct pset_name {
  char name[MPI_MAX_PSET_NAME_LEN];
  int pset_size;  // just a convenience field
};
typedef struct pset_name pset_name;
#define P
#define T pset_name
int pset_name_compare(pset_name *a, pset_name *b);
#include <set.h>

int set_pset_name_find_by_name(set_pset_name *set, char const *name,
                               pset_name **res);

// set_process_state
struct process_state {
  int process_id;
  bool active;    // currently always true (idle processes aren't tracked)
  bool reserved;  // might be relevant once we allow more than one "rc-view"
                  // application to be scheduled
  bool pending_shutdown;
  bool dynamic_start;
  int origin_rc_tag;  // can be looked in rc_table

  MPI_Info origin_rc_info;

  set_pset_name psets_containing;  // name of psets that contain the state
};
typedef struct process_state process_state;
#undef P
#define T process_state
void process_state_free(process_state *ps);
process_state process_state_copy(process_state *ps);
int process_state_compare(process_state *a, process_state *b);
#include <set.h>
int set_process_state_find_by_id(set_process_state *set, int process_id,
                                 process_state **res);

#endif
