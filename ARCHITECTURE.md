## How to navigate the code

The main code is written in C and can be found in the `src` directory.

There you can find the two header files that will also be accessible by the end user of the library: `mpidynres.h` and `mpidynres_sim.h`.

While the latter contains code that should be called by the external bootstrapper of the simulated environment, the former contains functions that should only be called during the simulated environment.

During the simulation, the process of rank 0 will act as the process manager/scheduler and will mostly call code from C files beginning with `schedul...`.

The file `mpidynres.c` contains the implementation of the functions defined in the `mpidynres.h`.
Besides `MPIDYNRES_Info_create_strings` and `MPI_Group_from_session_pset`, they mostly serialize their arguments, and communicate with the resource manager using functions and datastructures defined in `comm.h`.

The scheduler needs a lot of datastructures to hold its own state and track process environments and states. The library is using the 3rd-party library ctl. It is included in the `3rdparty/ctl` directory.
Datatypes are declared in the `scheduler_datatypes` sources.

The files `logging.{c,h}` and `util.h` contain useful macros and logging utility but not a lot of main logic.

The file `scheduler_mgmt.h` contains a generic interface for a scheduling manager implementation. Two example implementations are included in the `src/managers` directory.
