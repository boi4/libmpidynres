module mpidynres_sim

use, intrinsic :: iso_c_binding
use mpi


implicit none

  type, bind(C) :: MPIDYNRES_SIM_CONFIG
    integer :: base_communicator
    integer :: manager_config
  end type MPIDYNRES_SIM_CONFIG

  abstract interface
    subroutine mpidynres_main_func() bind(c)
    end subroutine
  end interface

contains

  subroutine MPIDYNRES_SIM_GET_DEFAULT_CONFIG(o_config)
    type(MPIDYNRES_SIM_CONFIG), intent(out) :: o_config

    INTERFACE
       SUBROUTINE FMPIDYNRES_SIM_GET_DEFAULT_CONFIG(o_config) bind(C, name="FMPIDYNRES_SIM_get_default_config")
         import MPIDYNRES_SIM_CONFIG
         type(MPIDYNRES_SIM_CONFIG), INTENT(OUT):: o_config
       END SUBROUTINE FMPIDYNRES_SIM_GET_DEFAULT_CONFIG
    END INTERFACE

    call FMPIDYNRES_SIM_GET_DEFAULT_CONFIG(o_config)
  end subroutine MPIDYNRES_SIM_GET_DEFAULT_CONFIG


  subroutine MPIDYNRES_SIM_START(i_config, i_sim_main)
    type(MPIDYNRES_SIM_CONFIG), intent(in) :: i_config
    procedure(mpidynres_main_func) :: i_sim_main

    INTERFACE
       SUBROUTINE FMPIDYNRES_SIM_START(i_config, i_sim_main) bind(C, name="FMPIDYNRES_SIM_start")
         import MPIDYNRES_SIM_CONFIG
         import :: c_funptr
         type(MPIDYNRES_SIM_CONFIG), INTENT(IN):: i_config
         type(c_funptr), VALUE, INTENT(IN):: i_sim_main
       END SUBROUTINE FMPIDYNRES_SIM_START
    END INTERFACE

    call FMPIDYNRES_SIM_START(i_config, C_FUNLOC(i_sim_main))
  end subroutine MPIDYNRES_SIM_START

end module mpidynres_sim
