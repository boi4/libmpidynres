module mpidynres

use, intrinsic :: iso_c_binding
use mpi, only: MPI_MAX_INFO_KEY

implicit none

  ! type, bind(C) :: MPIDYNRES_SIM_CONFIG
  !   integer :: base_communicator
  !   integer :: manager_config
  ! end type MPIDYNRES_SIM_CONFIG

  ! abstract interface
  !   subroutine mpidynres_main_func() bind(c)
  !   end subroutine
  ! end interface

  ! constants exported as symbols
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
    character(len=*), intent(in) :: fstring
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










  subroutine MPI_SESSION_INIT(info, errhandler, session, ierror)
    integer                       , intent(in)  :: info
    integer                       , intent(in)  :: errhandler
    type(c_ptr)                   , intent(out) :: session
    integer             , optional, intent(out) :: ierror

    integer :: result

    interface
      function FMPI_SESSION_INIT(info, errhandler, session) bind(C, name="FMPI_Session_init")
        import c_int
        import c_ptr
        integer     , intent(in) :: info
        integer     , intent(in) :: errhandler
        type(c_ptr) , intent(out):: session
        integer(kind=c_int)              :: FMPI_SESSION_INIT
      end function FMPI_SESSION_INIT
    end interface

    result = FMPI_SESSION_INIT(info, errhandler, session)
    if (present(ierror)) then
      ierror = result
    end if
  end subroutine MPI_SESSION_INIT



  subroutine MPI_SESSION_FINALIZE(session, ierror)
    type(c_ptr)                   , intent(inout) :: session
    integer             , optional, intent(out)   :: ierror

    integer :: result

    interface
      function FMPI_SESSION_FINALIZE(session) bind(C, name="FMPI_Session_finalize")
        import c_int
        import c_ptr
        type(c_ptr), intent(inout):: session
        integer(kind=c_int) :: FMPI_SESSION_FINALIZE
      end function FMPI_SESSION_FINALIZE
    end interface

    result = FMPI_SESSION_FINALIZE(session)
    if (present(ierror)) then
      ierror = result
    end if
  end subroutine MPI_SESSION_FINALIZE


  subroutine MPI_SESSION_GET_INFO(session, info_used, ierror)
    type(c_ptr)                   , intent(in)  :: session
    integer                , intent(out) :: info_used
    integer             , optional, intent(out) :: ierror

    integer :: result

    interface
      function FMPI_SESSION_GET_INFO(session, info) bind(C, name="FMPI_Session_get_info")
        import c_int
        import c_ptr
        type(c_ptr)   , intent(in) :: session
        integer, intent(out):: info
        integer(kind=c_int) :: FMPI_SESSION_GET_INFO
      end function FMPI_SESSION_GET_INFO
    end interface

    result = FMPI_SESSION_GET_INFO(session, info_used)
    if (present(ierror)) then
      ierror = result
    end if
  end subroutine MPI_SESSION_GET_INFO


  subroutine MPI_SESSION_GET_PSETS(session, info, psets, ierror)
    type(c_ptr)                   , intent(in)  :: session
    integer                , intent(in)  :: info
    integer                , intent(out) :: psets
    integer             , optional, intent(out) :: ierror

    integer :: result

    interface
      function FMPI_SESSION_GET_PSETS(session, info, psets) bind(C, name="FMPI_Session_get_psets")
        import c_int
        import c_ptr
        type(c_ptr)   , intent(in) :: session
        integer, intent(in) :: info
        integer, intent(out):: psets
        integer(kind=c_int) :: FMPI_SESSION_GET_PSETS
      end function FMPI_SESSION_GET_PSETS
    end interface

    result = FMPI_SESSION_GET_PSETS(session, info, psets)
    if (present(ierror)) then
      ierror = result
    end if
  end subroutine MPI_SESSION_GET_PSETS


  ! TODO: check if we can say "max len" for character
  subroutine MPI_SESSION_GET_PSET_INFO(session, pset_name, info, ierror)
    type(c_ptr)                 , intent(in)  :: session
    character(len=*)            , intent(in)  :: pset_name
    integer              , intent(out) :: info
    integer           , optional, intent(out) :: ierror

    integer :: result

    interface
      function FMPI_SESSION_GET_PSET_INFO(session, pset_name, info) bind(C, name="FMPI_Session_get_pset_info")
        import c_int
        import c_ptr
        import c_char
        type(c_ptr)                         , intent(in)  :: session
        character(kind=c_char), dimension(*), intent(in)  :: pset_name
        integer                      , intent(out) :: info
        integer(kind=c_int) :: FMPI_SESSION_GET_PSET_INFO
      end function FMPI_SESSION_GET_PSET_INFO
    end interface

    result = FMPI_SESSION_GET_PSET_INFO(session, F2C_STRING(pset_name), info)
    if (present(ierror)) then
      ierror = result
    end if
  end subroutine MPI_SESSION_GET_PSET_INFO



  subroutine MPI_GROUP_FROM_SESSION_PSET(session, pset_name, newgroup, ierror)
    type(c_ptr)                 , intent(in)  :: session
    character(len=*)            , intent(in)  :: pset_name
    integer             , intent(out) :: newgroup
    integer           , optional, intent(out) :: ierror

    integer :: result

    interface
      function FMPI_GROUP_FROM_SESSION_PSET(session, pset_name, newgroup) bind(C, name="FMPI_Group_from_session_pset")
        import c_int
        import c_ptr
        import c_char
        type(c_ptr)                         , intent(in) :: session
        character(kind=c_char), dimension(*), intent(in) :: pset_name
        integer                     , intent(out):: newgroup
        integer(kind=c_int)                              :: FMPI_GROUP_FROM_SESSION_PSET
      end function FMPI_GROUP_FROM_SESSION_PSET
    end interface

    result = FMPI_GROUP_FROM_SESSION_PSET(session, F2C_STRING(pset_name), newgroup)
    if (present(ierror)) then
      ierror = result
    end if
  end subroutine MPI_GROUP_FROM_SESSION_PSET



  subroutine MPI_COMM_CREATE_FROM_GROUP(group, stringtag, info, errhandler, newcomm, ierror)
    integer             , intent(in)  :: group
    character(len=*)            , intent(in)  :: stringtag
    integer              , intent(in)  :: info
    integer        , intent(in)  :: errhandler
    integer              , intent(out) :: newcomm
    integer           , optional, intent(out) :: ierror

    integer :: result

    interface
      function FMPI_COMM_CREATE_FROM_GROUP(group, stringtag, info, errhandler, newcomm) &
            bind(C, name="FMPI_Comm_create_from_group")
        import c_char
        import c_int
        integer                     , intent(in) :: group
        character(kind=c_char), dimension(*), intent(in) :: stringtag
        integer                      , intent(in) :: info
        integer                , intent(in) :: errhandler
        integer                      , intent(out):: newcomm
        integer(kind=c_int)                              :: FMPI_COMM_CREATE_FROM_GROUP
      end function FMPI_COMM_CREATE_FROM_GROUP
    end interface

    result = FMPI_COMM_CREATE_FROM_GROUP(group, F2C_STRING(stringtag), info, errhandler, newcomm)
    if (present(ierror)) then
      ierror = result
    end if
  end subroutine MPI_COMM_CREATE_FROM_GROUP

  subroutine MPIDYNRES_PSET_CREATE_OP(session, hints, pset1, pset2, op, pset_result, ierror)
    type(c_ptr)                 , intent(in)  :: session
    integer              , intent(in)  :: hints
    character(len=*)            , intent(in)  :: pset1
    character(len=*)            , intent(in)  :: pset2
    integer(kind=c_int)         , intent(in)  :: op
    character(len=*)            , intent(out) :: pset_result
    integer           , optional, intent(out) :: ierror

    integer :: result

    character(kind=c_char), dimension(MPI_MAX_PSET_NAME_LEN) :: pset_result_c
    integer :: i

    interface
      function FMPIDYNRES_PSET_CREATE_OP(session, hints, pset1, pset2, op, pset_result) &
            bind(C, name="FMPIDYNRES_pset_create_op")
        import c_int
        import c_ptr
        import c_char
        type(c_ptr)                                       :: session
        integer                                    :: hints
        character(kind=c_char), dimension(*), intent(in)  :: pset1
        character(kind=c_char), dimension(*), intent(in)  :: pset2
        integer(kind=c_int)                               :: op
        character(kind=c_char), dimension(*), intent(out) :: pset_result
        integer(kind=c_int)                               :: FMPIDYNRES_PSET_CREATE_OP
      end function FMPIDYNRES_PSET_CREATE_OP
    end interface

    result = FMPIDYNRES_PSET_CREATE_OP(session, hints, F2C_STRING(pset1), F2C_STRING(pset2), op, pset_result_c)
    if (present(ierror)) then
      ierror = result
    end if

    if (result == 0) then
      do i = 1, MPI_MAX_PSET_NAME_LEN
        pset_result(i:i) = pset_result_c(i)
        if (pset_result_c(i) == C_NULL_CHAR) exit
      end do
    end if

  end subroutine MPIDYNRES_PSET_CREATE_OP

  subroutine MPIDYNRES_PSET_FREE(session, pset_name, ierror)
    type(c_ptr)                 , intent(in)  :: session
    character(len=*)            , intent(in)  :: pset_name
    integer           , optional, intent(out) :: ierror

    integer :: result

    interface
      function FMPIDYNRES_PSET_FREE(session, pset_name) bind(C, name="FMPIDYNRES_pset_free")
        import c_int
        import c_ptr
        import c_char
        type(c_ptr)                         , intent(in) :: session
        character(kind=c_char), dimension(*), intent(in) :: pset_name
        integer(kind=c_int)                              :: FMPIDYNRES_PSET_FREE
      end function FMPIDYNRES_PSET_FREE
    end interface

    result = FMPIDYNRES_PSET_FREE(session, F2C_STRING(pset_name))
    if (present(ierror)) then
      ierror = result
    end if
  end subroutine MPIDYNRES_PSET_FREE


  subroutine MPIDYNRES_ADD_SCHEDULING_HINTS(session, hints, answer, ierror)
    type(c_ptr)                 , intent(in)  :: session
    integer              , intent(in)  :: hints
    integer              , intent(out) :: answer
    integer           , optional, intent(out) :: ierror

    integer :: result

    interface
      function FMPIDYNRES_ADD_SCHEDULING_HINTS(session, hints, answer) bind(C, name="FMPIDYNRES_add_scheduling_hints")
        import c_int
        import c_ptr
        type(c_ptr)   , intent(in)  :: session
        integer, intent(in)  :: hints
        integer, intent(out) :: answer
        integer(kind=c_int)         :: FMPIDYNRES_ADD_SCHEDULING_HINTS
      end function FMPIDYNRES_ADD_SCHEDULING_HINTS
    end interface

    result = FMPIDYNRES_ADD_SCHEDULING_HINTS(session, hints, answer)
    if (present(ierror)) then
      ierror = result
    end if
  end subroutine MPIDYNRES_ADD_SCHEDULING_HINTS


  subroutine MPIDYNRES_RC_GET(session, rc_type, delta_pset, tag, info, ierror)
    type(c_ptr)                 , intent(in)  :: session
    integer                     , intent(out) :: rc_type
    character(len=*)            , intent(out) :: delta_pset
    integer                     , intent(out) :: tag
    integer                     , intent(out) :: info
    integer           , optional, intent(out) :: ierror

    integer :: result

    character(kind=c_char), dimension(MPI_MAX_PSET_NAME_LEN) :: delta_pset_c
    integer :: i

    interface
      function FMPIDYNRES_RC_GET(session, rc_type, delta_pset, tag, info) &
            bind(C, name="FMPIDYNRES_RC_get")
        import c_ptr
        import c_int
        import c_char
        type(c_ptr)                         , intent(in)  :: session
        integer(kind=c_int)                 , intent(out) :: rc_type
        character(kind=c_char), dimension(*), intent(out) :: delta_pset
        integer(kind=c_int)                 , intent(out) :: tag
        integer                             , intent(out) :: info
        integer(kind=c_int)                               :: FMPIDYNRES_RC_GET
      end function FMPIDYNRES_RC_GET
    end interface

    result = FMPIDYNRES_RC_GET(session, rc_type, delta_pset_c, tag, info)
    if (present(ierror)) then
      ierror = result
    end if

    if (result == 0) then
      do i = 1, MPI_MAX_PSET_NAME_LEN
        delta_pset(i:i) = delta_pset_c(i)
        if (delta_pset_c(i) == C_NULL_CHAR) exit
      end do
    end if

  end subroutine MPIDYNRES_RC_GET



  subroutine MPIDYNRES_RC_ACCEPT(session, tag, info, ierror)
    type(c_ptr)                 , intent(in)  :: session
    integer                     , intent(in) :: tag
    integer                     , intent(in) :: info
    integer           , optional, intent(out) :: ierror

    integer :: result

    interface
      function FMPIDYNRES_RC_ACCEPT(session, tag, info) &
            bind(C, name="FMPIDYNRES_RC_accept")
        import c_ptr
        import c_int
        import c_char
        type(c_ptr)                         , intent(in) :: session
        integer(kind=c_int)                 , intent(in) :: tag
        integer                             , intent(in) :: info
        integer(kind=c_int)                              :: FMPIDYNRES_RC_ACCEPT
      end function FMPIDYNRES_RC_ACCEPT
    end interface

    result = FMPIDYNRES_RC_ACCEPT(session, tag, info)
    if (present(ierror)) then
      ierror = result
    end if

  end subroutine MPIDYNRES_RC_ACCEPT


  subroutine MPIDYNRES_EXIT()
      interface
        subroutine FMPIDYNRES_EXIT() bind(C, name="FMPIDYNRES_exit")
        end subroutine FMPIDYNRES_EXIT
      end interface

    call FMPIDYNRES_EXIT()
  end subroutine MPIDYNRES_EXIT

end module mpidynres
