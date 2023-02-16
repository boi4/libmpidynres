MODULE app
use mpi_f08
use, intrinsic :: iso_c_binding

contains

character(len=64) function itoa(n) result(s)
    integer, intent(in) :: n
    write (s, *) n
    s = adjustl(s)
end function itoa

subroutine test() bind(c)
  use mpidynres_f08
  implicit none

  integer :: ierror
  character(len=100) :: s
  type(MPI_Info) :: info
  type(c_ptr) :: session
  type(MPI_Group) :: group
  type(MPI_Comm) :: comm
  logical :: flag
  integer :: size, myrank;
  character(len=MPI_MAX_PSET_NAME_LEN) :: pset_name

  print *, "Hello from Fortran 08 Application running on mpidynres :-)"

  call MPI_INFO_CREATE(info, ierror)
  call MPI_INFO_SET(info, "a", "b", ierror)

  call MPI_SESSION_INIT(info, MPI_ERRHANDLER_NULL, session)
  call MPI_INFO_FREE(info, ierror)


  call MPI_SESSION_GET_INFO(session, info)
  call MPI_INFO_GET(info, "a", 100, s, flag, ierror)
  call MPI_INFO_FREE(info, ierror)
  print *, "MPI_SESSION_GET_INFO test: ", s


  call MPI_SESSION_GET_PSETS(session, MPI_INFO_NULL, info)
  call MPI_INFO_GET(info, "mpi://WORLD", 100, s, flag, ierror)
  call MPI_INFO_FREE(info, ierror)
  print *, "MPI_SESSION_GET_PSETS test: ", s


  call MPI_SESSION_GET_PSET_INFO(session, "mpi://WORLD", info)
  call MPI_INFO_GET(info, "mpidynres_name", 100, s, flag, ierror)
  call MPI_INFO_FREE(info, ierror)
  print *, "MPI_SESSION_GET_PSET_INFO test: ", s

  call MPI_GROUP_FROM_SESSION_PSET(session, "mpi://WORLD", group)
  call MPI_COMM_CREATE_FROM_GROUP(group, "", MPI_INFO_NULL, MPI_ERRHANDLER_NULL, comm)
  call MPI_GROUP_FREE(group, ierror)

  call MPI_COMM_SIZE(comm, size, ierror)
  call MPI_COMM_RANK(comm, myrank, ierror)

  print *, "Comm size: ", size
  print *, "My Rank: ", myrank


  call MPIDYNRES_PSET_CREATE_OP(session, mpi_info_null, "mpi://WORLD", "mpi://SELF", MPIDYNRES_PSET_DIFFERENCE, pset_name)
  print *, "Created new pset: ", pset_name

  call MPI_SESSION_GET_PSET_INFO(session, pset_name, info)
  call MPI_INFO_GET(info, "mpidynres_name", 100, s, flag, ierror)
  call MPI_INFO_FREE(info, ierror)
  print *, "MPI_SESSION_GET_PSET_INFO test with new pset: ", s
  call MPIDYNRES_PSET_FREE(session, pset_name)


  call MPI_BARRIER(comm, ierror)
  call MPI_COMM_FREE(comm, ierror)

  call MPI_SESSION_FINALIZE(session)

  call MPIDYNRES_EXIT()

end subroutine test

END MODULE app






PROGRAM main
  use mpidynres_sim_f08
  use app
  implicit none


  type(MPIDYNRES_SIM_CONFIG) :: config
  integer :: ierror,world_size
  procedure(mpidynres_main_func), pointer :: main_func => test

  call MPI_INIT(ierror)
  !call MPIDYNRES_SIM_GET_DEFAULT_CONFIG(config)
  config%base_communicator = MPI_COMM_WORLD
  call MPI_INFO_CREATE(config%manager_config, ierror)
  ! call MPI_INFO_SET(config%manager_config, "manager_initial_number_random", "yes")
  call MPI_COMM_SIZE(MPI_COMM_WORLD, world_size, ierror)
  call MPI_INFO_SET(config%manager_config, "manager_initial_number", itoa(world_size - 1), ierror)

  call MPIDYNRES_SIM_START(config, main_func)

  call MPI_INFO_FREE(config%manager_config, ierror)
  call MPI_FINALIZE(ierror)
END PROGRAM main
