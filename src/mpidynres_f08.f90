module mpidynres_f08

use, intrinsic :: iso_c_binding
use mpi_f08;

! TODO: add intent to MPI.. routine arguments
! TODO: consistent capitalization etc
! TODO: add custom type for mpi session
! TODO: write function for string to c_string conversion

implicit none

  ! type, bind(C) :: MPIDYNRES_SIM_CONFIG
  !   integer :: base_communicator
  !   integer :: manager_config
  ! end type MPIDYNRES_SIM_CONFIG

  ! abstract interface
  !   subroutine mpidynres_main_func() bind(c)
  !   end subroutine
  ! end interface

  ! constants exported as symbold
  type(c_ptr), bind(C, name="MPI_SESSION_NULL") :: MPI_SESSION_NULL
  integer(kind=C_INT), bind(C, name="MPIDYNRES_INVALID_SESSION_ID") :: MPIDYNRES_INVALID_SESSION_ID

  ! manually defined constants
  integer :: MPI_MAX_PSET_NAME_LEN = MPI_MAX_INFO_KEY + 1
  integer :: MPIDYNRES_NO_ORIGIN_RC_TAG = -1

  ! enum MPIDYNRES_pset_op
  integer :: MPIDYNRES_PSET_UNION      = 0
  integer :: MPIDYNRES_PSET_INTERSECT  = 1
  integer :: MPIDYNRES_PSET_DIFFERENCE = 2

  ! enum MPIDYNRES_RC_type
  integer :: MPIDYNRES_RC_NONE = 0
  integer :: MPIDYNRES_RC_ADD  = 1
  integer :: MPIDYNRES_RC_SUB  = 2

contains


  ! helper function to convert a fortan string into a trimmed, null delimited c string
  function F2C_STRING(fstring) result(cstring)
    character(len=*), INTENT(IN) :: fstring
    character(kind=c_char), dimension(:),allocatable :: cstring
    integer :: i
    integer :: l
    l = LEN_TRIM(fstring)
    allocate ( cstring(l+1) )
    do i = 1, l
       cstring(i:i) = fstring(i:i)
    end do
    cstring(l+1:l+1) = C_NULL_CHAR
  end function F2C_STRING










  subroutine MPI_SESSION_INIT(info, errhandler, session)
    type(MPI_Info) :: info;
    type(MPI_Errhandler) :: errhandler;
    type(c_ptr) :: session;

      INTERFACE
        SUBROUTINE FMPI_SESSION_INIT(info, errhandler, session) bind(C, name="FMPI_Session_init")
          import MPI_Info
          import MPI_Errhandler
          import c_ptr
          type(MPI_Info), INTENT(IN):: info;
          type(MPI_Errhandler), INTENT(IN):: errhandler;
          type(c_ptr), INTENT(OUT):: session;
        END SUBROUTINE FMPI_SESSION_INIT
      END INTERFACE

    call FMPI_SESSION_INIT(info, errhandler, session)
  end subroutine MPI_SESSION_INIT



  subroutine MPI_SESSION_FINALIZE(session)
    type(c_ptr) :: session;

      INTERFACE
        SUBROUTINE FMPI_SESSION_FINALIZE(session) bind(C, name="FMPI_Session_finalize")
          import c_ptr
          type(c_ptr), INTENT(INOUT):: session;
        END SUBROUTINE FMPI_SESSION_FINALIZE
      END INTERFACE

    call FMPI_SESSION_FINALIZE(session)
  end subroutine MPI_SESSION_FINALIZE


  subroutine MPI_SESSION_GET_INFO(session, info)
    type(c_ptr) :: session;
    type(MPI_INFO) :: info;

      INTERFACE
        SUBROUTINE FMPI_SESSION_GET_INFO(session, info) bind(C, name="FMPI_Session_get_info")
          import c_ptr
          import MPI_INFO
          type(c_ptr), INTENT(IN):: session;
          type(MPI_INFO), INTENT(OUT):: info;
        END SUBROUTINE FMPI_SESSION_GET_INFO
      END INTERFACE

    call FMPI_SESSION_GET_INFO(session, info)
  end subroutine MPI_SESSION_GET_INFO


  subroutine MPI_SESSION_GET_PSETS(session, info, psets)
    type(c_ptr) :: session;
    type(MPI_INFO) :: info;
    type(MPI_INFO) :: psets;

      INTERFACE
        SUBROUTINE FMPI_SESSION_GET_PSETS(session, info, psets) bind(C, name="FMPI_Session_get_psets")
          import c_ptr
          import MPI_Info
          type(c_ptr), INTENT(IN):: session;
          type(MPI_INFO), INTENT(OUT):: info;
          type(MPI_INFO), INTENT(OUT):: psets;
        END SUBROUTINE FMPI_SESSION_GET_PSETS
      END INTERFACE

    call FMPI_SESSION_GET_PSETS(session, info, psets)
  end subroutine MPI_SESSION_GET_PSETS


  ! TODO: check if we can say "max len" for character
  subroutine MPI_SESSION_GET_PSET_INFO(session, pset_name, info)
    type(c_ptr) :: session;
    character(len=*), INTENT(IN) :: pset_name
    type(MPI_INFO) :: info

      INTERFACE
        SUBROUTINE FMPI_SESSION_GET_PSET_INFO(session, pset_name, info) bind(C, name="FMPI_Session_get_pset_info")
          import c_ptr
          import c_char
          import MPI_Info
          type(c_ptr), INTENT(IN):: session;
          character(kind=c_char), DIMENSION(*), INTENT(IN) :: pset_name
          type(MPI_INFO), INTENT(OUT):: info;
        END SUBROUTINE FMPI_SESSION_GET_PSET_INFO
      END INTERFACE

    call FMPI_SESSION_GET_PSET_INFO(session, F2C_STRING(pset_name), info)
  end subroutine MPI_SESSION_GET_PSET_INFO



  subroutine MPI_GROUP_FROM_SESSION_PSET(session, pset_name, newgroup)
    type(c_ptr) :: session;
    character(len=*), INTENT(IN) :: pset_name
    type(MPI_Group) :: newgroup;

      INTERFACE
        SUBROUTINE FMPI_GROUP_FROM_SESSION_PSET(session, pset_name, newgroup) bind(C, name="FMPI_Group_from_session_pset")
          import c_ptr
          import c_char
          import MPI_Group
          type(c_ptr), INTENT(IN):: session;
          character(kind=c_char), DIMENSION(*), INTENT(IN) :: pset_name
          type(MPI_Group), INTENT(OUT):: newgroup;
        END SUBROUTINE FMPI_GROUP_FROM_SESSION_PSET
      END INTERFACE
    call FMPI_GROUP_FROM_SESSION_PSET(session, F2C_STRING(pset_name), newgroup)
  end subroutine MPI_GROUP_FROM_SESSION_PSET



  subroutine MPI_COMM_CREATE_FROM_GROUP(group, stringtag, info, errhandler, newcomm)
    type(MPI_Group) :: group;
    character(len=*), INTENT(IN) :: stringtag
    type(MPI_Info) :: info;
    type(MPI_Errhandler) :: errhandler;
    type(MPI_Comm) :: newcomm;

      INTERFACE
        SUBROUTINE FMPI_COMM_CREATE_FROM_GROUP(group, stringtag, info, errhandler, newcomm) &
             bind(C, name="FMPI_Comm_create_from_group")
          import MPI_Group
          import c_char
          import MPI_Info
          import MPI_Errhandler
          import MPI_COmm
          type(MPI_Group), INTENT(IN):: group;
          character(kind=c_char), DIMENSION(*), INTENT(IN) :: stringtag
          type(MPI_Info), INTENT(IN):: info;
          type(MPI_Errhandler), INTENT(IN):: errhandler;
          type(MPI_Comm), INTENT(OUT):: newcomm;
        END SUBROUTINE FMPI_COMM_CREATE_FROM_GROUP
      END INTERFACE

    call FMPI_COMM_CREATE_FROM_GROUP(group, F2C_STRING(stringtag), info, errhandler, newcomm)
  end subroutine MPI_COMM_CREATE_FROM_GROUP

  subroutine MPIDYNRES_PSET_CREATE_OP(session, hints, pset1, pset2, op, pset_result)
    type(c_ptr) :: session
    type(MPI_Info) :: hints
    character(len=*), INTENT(IN) :: pset1
    character(len=*), INTENT(IN) :: pset2
    integer(kind=c_int) :: op
    character(len=*), INTENT(OUT) :: pset_result

    character(kind=c_char), dimension(MPI_MAX_PSET_NAME_LEN) :: pset_result_c
    integer :: i

      INTERFACE
        SUBROUTINE FMPIDYNRES_PSET_CREATE_OP(session, hints, pset1, pset2, op, pset_result) &
             bind(C, name="FMPIDYNRES_pset_create_op")
          import c_ptr
          import MPI_Info
          import c_char
          import c_int
          type(c_ptr) :: session
          type(MPI_Info) :: hints
          character(kind=c_char), DIMENSION(*), INTENT(IN) :: pset1
          character(kind=c_char), DIMENSION(*), INTENT(IN) :: pset2
          integer(kind=c_int) :: op
          character(kind=c_char), DIMENSION(*), INTENT(OUT) :: pset_result
        END SUBROUTINE FMPIDYNRES_PSET_CREATE_OP
      END INTERFACE

    call FMPIDYNRES_PSET_CREATE_OP(session, hints, F2C_STRING(pset1), F2C_STRING(pset2), op, pset_result_c)

    do i = 1, MPI_MAX_PSET_NAME_LEN
       pset_result(i:i) = pset_result_c(i)
    end do

  end subroutine MPIDYNRES_PSET_CREATE_OP

  subroutine MPIDYNRES_PSET_FREE(session, pset_name)
    type(c_ptr) :: session;
    character(len=*), INTENT(IN) :: pset_name

      INTERFACE
        SUBROUTINE FMPIDYNRES_PSET_FREE(session, pset_name) bind(C, name="FMPIDYNRES_pset_free")
          import c_ptr
          import c_char
          type(c_ptr), INTENT(IN):: session;
          character(kind=c_char), DIMENSION(*), INTENT(IN) :: pset_name
        END SUBROUTINE FMPIDYNRES_PSET_FREE
      END INTERFACE
    call FMPIDYNRES_PSET_FREE(session, F2C_STRING(pset_name))
  end subroutine MPIDYNRES_PSET_FREE


  subroutine MPIDYNRES_ADD_SCHEDULING_HINTS(session, hints, answer)
    type(c_ptr) :: session;
    type(MPI_Info) :: hints
    type(MPI_Info) :: answer

      INTERFACE
        SUBROUTINE FMPIDYNRES_ADD_SCHEDULING_HINTS(session, hints, answer) bind(C, name="FMPIDYNRES_add_scheduling_hints")
          import c_ptr
          import MPI_Info
          type(c_ptr), INTENT(IN):: session;
          type(MPI_Info) :: hints
          type(MPI_Info) :: answer
        END SUBROUTINE FMPIDYNRES_ADD_SCHEDULING_HINTS
      END INTERFACE
    call FMPIDYNRES_ADD_SCHEDULING_HINTS(session, hints, answer)
  end subroutine MPIDYNRES_ADD_SCHEDULING_HINTS


  subroutine MPIDYNRES_RC_GET(session, rc_type, delta_pset, tag, info)
    type(c_ptr) :: session
    integer :: rc_type
    character(len=*), INTENT(OUT) :: delta_pset
    integer :: tag
    type(MPI_Info) :: info

    character(kind=c_char), dimension(MPI_MAX_PSET_NAME_LEN) :: delta_pset_c
    integer :: i

      INTERFACE
        SUBROUTINE FMPIDYNRES_RC_GET(session, rc_type, delta_pset, tag, info) &
             bind(C, name="FMPIDYNRES_RC_get")
          import c_ptr
          import c_int
          import c_char
          import MPI_Info
          type(c_ptr) :: session
          integer(kind=c_int) :: rc_type
          character(kind=c_char), DIMENSION(*), INTENT(OUT) :: delta_pset
          integer(kind=c_int) :: tag
          type(MPI_Info) :: info
        END SUBROUTINE FMPIDYNRES_RC_GET
      END INTERFACE

    call FMPIDYNRES_RC_GET(session, rc_type, delta_pset_c, tag, info)

    do i = 1, MPI_MAX_PSET_NAME_LEN
       delta_pset(i:i) = delta_pset_c(i)
    end do

  end subroutine MPIDYNRES_RC_GET


  subroutine MPIDYNRES_EXIT()
      INTERFACE
        SUBROUTINE FMPIDYNRES_EXIT() bind(C, name="FMPIDYNRES_exit")
        END SUBROUTINE FMPIDYNRES_EXIT
      END INTERFACE

    call FMPIDYNRES_EXIT()
  end subroutine MPIDYNRES_EXIT

end module mpidynres_f08
