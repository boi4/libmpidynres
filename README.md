# libmpidynres

libmpidynres is a C library that provides a simulated MPI Sessions environment for MPI applications.

libmpidynres is NOT
* a real scalable runtime library
* a parallel program scheduler

## Building

### Build Requirements
 * a (C11) compiler
 * MPI headers + library
 * make
 * gnu coreutils or a similar POSIX environment (for find, cp, mkdir etc.)
 * rsync (for installation)

### Runtime Requirements
 * working MPI runtime
 
 

Please take a look at the Makefile and check if everything is correctly set to your liking.

For building, run `make`. This will build a dynamic library and the header files into a `build` directory.

To install the headers and the library into your system, run `make install`. You can adjust the installation directory by setting the `INSTALL_PREFIX` variable.

To build the doxygen html documentation, run `make doc`. To build examples run `make examples`.

If you want to build your examples against an installed version of libmpidynres, please remove the `-I` and `-L` flags in the Makefile.


## Usage

Please, first build the library as described above. If you want your compiler to find the header and library files automatically, you can also install the library into your system. Otherwise, take a look at the `example` target in the Makefile to see how to tell your compiler where to find the header and library files (use the `-I` and `-L` flags).

The library provides two header files:

 * *mpidynres_sim.h:* This header defines functions for setting up the simulated environment and for starting it.
 * *mpidynres.h:* This header defines the main interface functions for the simulated process for communicating with the simulated runtime.
 
Please take a look at the header files and examples to see available functions and structures.


### Debugging

You can define the `MPIDYNRES_DEBUG` environment variable to get debug output printed to the stdout. Note that this can become quite messy when you let all ranks output to the same terminal.

To track the process states, you can set `MPIDYNRES_STATELOG` to be a filename that libmpidynres will output colorful output to (use `/proc/self/1` for stdout under linux).

## Architecture

See [ARCHITECTURE.md](./ARCHITECTURE.md).
