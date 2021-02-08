#include "scheduler_datatypes.h"

#include "string.h"



int int_compare(int *a, int *b) {
  return (*a == *b) ? 0 : ((*a > *b) ? 1 : -1);
}

int pset_node_compare(pset_node *a, pset_node *b) {
  return strcmp(a->pset_name, b->pset_name);
}

int rc_node_compare(rc_node *a, rc_node *b) {
  return int_compare(&a->rc_tag, &b->rc_tag);
}

int process_state_compare(process_state *a, process_state *b) {
  return int_compare(&a->process_id, &b->process_id);
}



void pset_node_free(pset_node *pn) {
  set_int_free(&pn->pset);
  MPI_Info_free(&pn->pset_info);
  /*free(pn);*/
}

pset_node pset_node_copy(pset_node *pn) {
  pset_node res = *pn;
  res.pset = set_int_copy(&pn->pset);
  MPI_Info_dup(pn->pset_info, &res.pset_info);
  return res;
}


void rc_node_free(rc_node *rn) {
  set_int_free(&rn->pset);
}

rc_node rc_node_copy(rc_node *rn) {
  rc_node res = *rn;
  res.pset = set_int_copy(&rn->pset);
  return res;
}