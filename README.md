# libmpidynres

libmpidynres is a C library that provides a simple interface for dynamic computing resource changes during an MPI program execution.

libmpidynres is NOT

* a real scalable runtime library
* a parallel program scheduler (it can only schedule one program at a time)

## Building

### Build Requirements
 * a (C11) compiler
 * mpi headers + library
 * gnu make
 * gnu coreutils or a similar POSIX environment (for find, cp, mkdir etc.)
 * rsync (for installation)

### Runtime Requirements
 * mpi library + runtime
 
 

Please take a look at the Makefile and check if everything is correctly set to your liking.

For building, run `make`. This will build a dynamic library and the header files into a `build` directory.

To install the headers and the library into your system, run `make install` (TODO). You can adjust the installation directory by setting the `INSTALL_PREFIX` variable.


## Usage

Please, first build the library as described above. If you want your compiler to find the header and library files automatically, you can also install the library into your system. Otherwise, take a look at the `example` target in the Makefile to see how to tell your compiler where to find the header and library files (use the `-I` and `-L` flags).

The library provides three header files:

 * *mpidynres_sim.h:* This header defines functions for setting up the simulated environment and for starting it.
 * *mpidynres.h:* This header defines the main interface functions for the simulated process for communicating with the simulated runtime.
 * *mpidynres_cr_set.h* This header defines the *cr_set* struct. 
 
Please take a look at the header files to see available functions and structures.


## Comparision between the MPI Sessions Paper and libmpidynres

Please take a look at section 3 in the [MPI Sessions paper](https://dl.acm.org/doi/10.1145/2966884.2966915).

### Concepts


|**MPI Sessions** | **libmpidynres** |
|--------------|------------------|
| *Session* - A self contained, scalable MPI environment | *-* - libmpidynres currently simulates a single session |
| *Group* - A set of computing resources contained in a session | *Computing Resource Set* - A specification of a computing resource set |
| *MPI\_Session/MPI\_Group* - Handles to the current environment/group | *URI* - A humand-readable handle to a *Computing Resource Set* |

### Initialization and Finalization

|**MPI Sessions** | **libmpidynres** |
|--------------|------------------|
|*MPI\_Session_init* Creates new Session and returns *MPI\_Session* object |*-* Processes are part of a Session when started|
|*MPI\_Session\_get_names* - get runtime names of computing resources |*MPIDYNRES
|*MPI\_Session\_get\_info* - get runtime info about current session |*MPIDYNRES\_get\_init\_info* - Obtain information about the starting environment of a process |

### Scaling & Communicators

|**MPI Sessions** | **libmpidynres** |
|--------------|------------------|
|*MPI_Group_create_session* - Create new scalable group |*-* Processes are part of a Computing resource set when started |
|*-* - It is not described how groups can be scaled with **available** resources |*MPIDYNRES\_URI\_create\_op* - Create a new resource set for existing available resource sets |
|*MPI\_Comm\_create\_group\_X* - create new Communicator from group |*MPIDYNRES\_Comm\_create\_URI* - Create a communicator with all resources that URI is a handle to |


### Communication with runtime about resource changes

|**MPI Sessions** | **libmpidynres** |
|--------------|------------------|
|*-* - Communication wiht the runtime about resource changes are not described (Maybe with get\_info) |*MPIDYNRES\_RC\_fetch/MPIDYNRES\_RC\_accept* - Ask and accept resource changes. An URI and a tag can be passed when accepting a *Resource Change* |

### Scalable topologies

*libmpidynres* does not support topologies or similar configuration for now.
