#ifndef SCHEDULER_HANDLERS_H
#define SCHEDULER_HANDLERS_H

void MPIDYNRES_scheduler_handle_worker_done(MPIDYNRES_scheduler *scheduler,
                                            MPI_Status *status);



void MPIDYNRES_scheduler_handle_session_create(MPIDYNRES_scheduler *scheduler,
                                               MPI_Status *status);

void MPIDYNRES_scheduler_handle_session_info(MPIDYNRES_scheduler *scheduler,
                                             MPI_Status *status);

void MPIDYNRES_scheduler_handle_session_finalize(MPIDYNRES_scheduler *scheduler,
                                                 MPI_Status *status);




void MPIDYNRES_scheduler_handle_get_psets(MPIDYNRES_scheduler *scheduler,
                                          MPI_Status *status);

void MPIDYNRES_scheduler_handle_pset_info(MPIDYNRES_scheduler *scheduler,
                                          MPI_Status *status, int strsize);

void MPIDYNRES_scheduler_handle_pset_free(MPIDYNRES_scheduler *scheduler, MPI_Status *status,
                                          MPIDYNRES_pset_free_msg *pset_free_msg);



void MPIDYNRES_scheduler_handle_pset_lookup(MPIDYNRES_scheduler *scheduler,
                                            MPI_Status *status,
                                            int name_strsize);


// TODO
void MPIDYNRES_scheduler_handle_pset_op(MPIDYNRES_scheduler *scheduler,
                                        MPI_Status *status,
                                        MPIDYNRES_pset_op_msg *pset_op_msg);



// TODO
void MPIDYNRES_scheduler_handle_rc(MPIDYNRES_scheduler *scheduler,
                                   MPI_Status *status);

// TODO
void MPIDYNRES_scheduler_handle_rc_accept(MPIDYNRES_scheduler *scheduler, MPI_Status *status,
                                          MPIDYNRES_RC_accept_msg *rc_accept_msg);

#endif
