# libmpidynres

libmpidynres is a C library that provides a simulation layer for dynamic computing resource changes during an MPI program execution.

libmpidynres is NOT

* a real scalable runtime library
* a parallel program scheduler (it can only schedule one program at a time)

## Building

### Build Requirements
 * a (C11) compiler
 * mpi headers + library
 * make
 * gnu coreutils or a similar POSIX environment (for find, cp, mkdir etc.)
 * rsync (for installation)

### Runtime Requirements
 * mpi library + runtime
 
 

Please take a look at the Makefile and check if everything is correctly set to your liking.

For building, run `make`. This will build a dynamic library and the header files into a `build` directory.

To install the headers and the library into your system, run `make install`. You can adjust the installation directory by setting the `INSTALL_PREFIX` variable.


## Usage

Please, first build the library as described above. If you want your compiler to find the header and library files automatically, you can also install the library into your system. Otherwise, take a look at the `example` target in the Makefile to see how to tell your compiler where to find the header and library files (use the `-I` and `-L` flags).

The library provides two header files:

 * *mpidynres_sim.h:* This header defines functions for setting up the simulated environment and for starting it.
 * *mpidynres.h:* This header defines the main interface functions for the simulated process for communicating with the simulated runtime.
 
Please take a look at the header files to see available functions and structures.

## Architecture

See [ARCHITECTURE.md](./ARCHITECTURE.md).


### Scalable topologies

*libmpidynres* does not support topologies or similar configuration for now.
