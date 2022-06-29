#include "mpidynres_fortran.h"
#include "util.h"

#include <string.h>

static int MPIDYNRES_SIM_f2c_config(FMPIDYNRES_SIM_config const *i_fconfig,
                                    MPIDYNRES_SIM_config *o_cconfig) {
    o_cconfig->base_communicator = MPI_Comm_f2c(i_fconfig->base_communicator);
    o_cconfig->manager_config    = MPI_Info_f2c(i_fconfig->manager_config);
    return 0;
}

static int MPIDYNRES_SIM_c2f_config(MPIDYNRES_SIM_config const *i_cconfig,
                                    FMPIDYNRES_SIM_config *o_fconfig) {
    o_fconfig->base_communicator = MPI_Comm_c2f(i_cconfig->base_communicator);
    o_fconfig->manager_config    = MPI_Info_c2f(i_cconfig->manager_config);
    return 0;
}


/*
 * TODO: assert that mpi is initialized
 */
int FMPIDYNRES_SIM_get_default_config(FMPIDYNRES_SIM_config *o_fconfig) {
  MPIDYNRES_SIM_config default_c_config = {0};
  int err = 0;
  err = MPIDYNRES_SIM_get_default_config(&default_c_config);
  if (err) {
    return err;
  }

  err = MPIDYNRES_SIM_c2f_config(&default_c_config, o_fconfig);
  if (err) {
    return err;
  }

  return 0;
}

int FMPIDYNRES_SIM_start(FMPIDYNRES_SIM_config *i_config,
                         int i_sim_main(int, char**)) {
  int err = 0;
  MPIDYNRES_SIM_config config = {0};
  err = MPIDYNRES_SIM_f2c_config(i_config, &config);
  if (err) {
    return err;
  }

  return MPIDYNRES_SIM_start(config, 0, NULL, *i_sim_main);
}


int FMPI_Session_init(MPI_Fint *info, MPI_Fint *errhandler,
                      MPI_Session *session) {
  MPI_Info c_info;
  MPI_Errhandler c_errhandler;
  c_info = MPI_Info_f2c(*info);
  c_errhandler = MPI_Errhandler_f2c(*errhandler);
  return MPI_Session_init(c_info, c_errhandler, session);
}


int FMPI_Session_finalize(MPI_Session *session) {
  return MPI_Session_finalize(session);
}


int FMPI_Session_get_info(MPI_Session *session, MPI_Fint *info_used) {
  MPI_Info c_info_used;
  int err;
  c_info_used = MPI_Info_f2c(*info_used);
  err =  MPI_Session_get_info(*session, &c_info_used);
  if (err) {
    return err;
  }
  *info_used = MPI_Info_c2f(c_info_used);
  return 0;
}


int FMPI_Session_get_psets(MPI_Session *session, MPI_Fint *info, MPI_Fint *psets) {
  MPI_Info c_info;
  MPI_Info c_psets;
  int err;
  c_info = MPI_Info_f2c(*info);
  c_psets = MPI_Info_f2c(*psets);
  err = MPI_Session_get_psets(*session, c_info, &c_psets);
  if (err) {
    return err;
  }
  *psets = MPI_Info_c2f(c_psets);
  return 0;
}


int FMPI_Session_get_pset_info(MPI_Session *session, char const *pset_name,
                               MPI_Fint *info) {
  MPI_Info c_info;
  int err;
  err = MPI_Session_get_pset_info(*session, pset_name, &c_info);
  if (err) {
    return err;
  }
  *info = MPI_Info_c2f(c_info);
  return 0;
}

int FMPI_Group_from_session_pset(MPI_Session *session, const char *pset_name,
                                 MPI_Fint *newgroup) {
  int err;
  MPI_Group c_newgroup;
  err = MPI_Group_from_session_pset(*session, pset_name, &c_newgroup);
  if (err) {
    return err;
  }
  *newgroup = MPI_Group_c2f(c_newgroup);
  return 0;
}

int FMPI_Comm_create_from_group(MPI_Fint *group, const char *stringtag,
                                MPI_Fint *info, MPI_Fint *errhandler, MPI_Fint *newcomm) {
  int err;
  MPI_Info c_info;
  MPI_Group c_group;
  MPI_Errhandler c_errhandler;

  MPI_Comm c_newcomm;
  c_group = MPI_Group_f2c(*group);
  c_info = MPI_Info_f2c(*info);
  c_errhandler = MPI_Errhandler_f2c(*errhandler);
  err = MPI_Comm_create_from_group(c_group, stringtag, c_info, c_errhandler, &c_newcomm);
  if (err) {
    return err;
  }
  *newcomm = MPI_Comm_c2f(c_newcomm);
  return 0;
}



int FMPIDYNRES_pset_create_op(MPI_Session *session,
                             MPI_Fint *hints,
                             char const pset1[],
                             char const pset2[],
                             MPIDYNRES_pset_op *op,
                             char pset_result[MPI_MAX_PSET_NAME_LEN]) {
  MPI_Info c_hints;
  int err;
  c_hints = MPI_Info_f2c(*hints);
  err = MPIDYNRES_pset_create_op(*session, c_hints, pset1, pset2, *op, pset_result);
  if (err) {
    return err;
  }
  memset(pset_result + strlen(pset_result), 0, MPI_MAX_PSET_NAME_LEN-strlen(pset_result) * sizeof(char));
  return 0;
}


int FMPIDYNRES_pset_free(MPI_Session *session, char i_pset_name[MPI_MAX_PSET_NAME_LEN]) {
  return MPIDYNRES_pset_free(*session, i_pset_name);
}


int FMPIDYNRES_add_scheduling_hints(MPI_Session *session, MPI_Fint *hints, MPI_Fint *answer) {
  MPI_Info c_hints;
  MPI_Info c_answer;
  int err;
  c_hints = MPI_Info_f2c(*hints);
  err = MPIDYNRES_add_scheduling_hints(*session, c_hints, &c_answer);
  if (err) {
    return err;
  }
  *answer = MPI_Info_c2f(c_answer);
  return 0;
}


int FMPIDYNRES_RC_get(MPI_Session *session,
                     MPIDYNRES_RC_type *rc_type,
                     char delta_pset[MPI_MAX_PSET_NAME_LEN],
                     MPIDYNRES_RC_tag *tag,
                      MPI_Fint *info) {
  MPI_Info c_info;
  int err;
  err = MPIDYNRES_RC_get(*session, rc_type, delta_pset, tag, &c_info);
  if (err) {
    return err;
  }
  *info = MPI_Info_c2f(c_info);
  memset(delta_pset + strlen(delta_pset), 0, MPI_MAX_PSET_NAME_LEN-strlen(delta_pset) * sizeof(char));
  return 0;
}


int FMPIDYNRES_RC_accept(MPI_Session *session, MPIDYNRES_RC_tag *tag,
                         MPI_Fint *info) {
  MPI_Info c_info;
  c_info = MPI_Info_f2c(*info);
  return MPIDYNRES_RC_accept(*session, *tag, c_info);
}


void FMPIDYNRES_exit() {
  MPIDYNRES_exit();
}
