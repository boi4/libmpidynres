#include "scheduler_datatypes.h"

#include "string.h"


int int_compare(int *a, int *b) {
  return (*a == *b) ? 0 : ((*a > *b) ? 1 : -1);
}



// PSET NODE

int pset_node_compare(pset_node *a, pset_node *b) {
  return strcmp(a->pset_name, b->pset_name);
}

void pset_node_free(pset_node *pn) {
  set_int_free(&pn->pset);
  if (pn->pset_info != MPI_INFO_NULL) {
    MPI_Info_free(&pn->pset_info);
  }
}

pset_node pset_node_copy(pset_node *pn) {
  pset_node res = *pn;
  strcpy(res.pset_name, pn->pset_name);
  res.pset = set_int_copy(&pn->pset);
  if (pn->pset_info == MPI_INFO_NULL) {
    res.pset_info = MPI_INFO_NULL;
  } else {
    MPI_Info_dup(pn->pset_info, &res.pset_info);
  }
  return res;
}


// helper function
int set_pset_node_find_by_name(set_pset_node *set, char const *name, pset_node **res) {
  pset_node search_key;
  strcpy(search_key.pset_name, name);

  // remove from scheduler map
  set_pset_node_node *psetn = set_pset_node_find(set, search_key);
   
  if (psetn == NULL) {
    *res = NULL;
    return 1;
  } else {
    *res = &(psetn->key);
    return 0;
  }
}




int rc_info_compare(rc_info *a, rc_info *b) {
  return int_compare(&a->rc_tag, &b->rc_tag);
}

int process_state_compare(process_state *a, process_state *b) {
  return int_compare(&a->process_id, &b->process_id);
}


process_state process_state_copy(process_state *ps) {
  process_state res;
  res = *ps;
  res.psets_containing = set_pset_name_copy(&ps->psets_containing);
  if (ps->origin_rc_info == MPI_INFO_NULL) {
    res.origin_rc_info = MPI_INFO_NULL;
  } else {
    MPI_Info_dup(ps->origin_rc_info, &res.origin_rc_info);
  }
  return res;
}

void process_state_free(process_state *ps) {
  set_pset_name_free(&ps->psets_containing);
  if (ps->origin_rc_info != MPI_INFO_NULL) {
    MPI_Info_free(&ps->origin_rc_info);
  }
}

int set_process_state_find_by_id(set_process_state *set, int process_id, process_state **res) {
  set_process_state_node *psn = set_process_state_find(set, (process_state){.process_id = process_id});
   
  if (psn == NULL) {
    *res = NULL;
    return 1;
  } else {
    *res = &(psn->key);
    return 0;
  }
}


// PSET NAME

int pset_name_compare(pset_name *a, pset_name *b) {
  return strcmp(a->name, b->name);
}

// helper function
int set_pset_name_find_by_name(set_pset_name *set, char const *name, pset_name **res) {
  pset_name search_key;
  strcpy(search_key.name, name);

  // remove from scheduler map
  set_pset_name_node *psetn = set_pset_name_find(set, search_key);
   
  if (psetn == NULL) {
    *res = NULL;
    return 1;
  } else {
    *res = &(psetn->key);
    return 0;
  }
}



void rc_info_free(rc_info *rn) {
  (void)rn;
  set_int_free(&rn->pset);
}

rc_info rc_info_copy(rc_info *rn) {
  rc_info res = *rn;
  res.pset = set_int_copy(&rn->pset);
  strcpy(res.new_pset_name, rn->new_pset_name);
  return res;
}



int set_rc_info_find_by_tag(set_rc_info *set, int tag, rc_info **res) {
  set_rc_info_node *rin = set_rc_info_find(set, (rc_info){.rc_tag = tag});
  if (rin == NULL) {
    *res = NULL;
    return 1;
  } else {
    *res = &(rin->key) ;
    return 0;
  }
}
