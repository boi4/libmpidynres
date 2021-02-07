#ifndef TEST_UTIL_H
#define TEST_UTIL_H

#include <mpi.h>
#include <stdlib.h>

#define COUNT_OF(x) \
  ((sizeof(x) / sizeof(0 [x])) / ((size_t)(!(sizeof(x) % sizeof(0 [x])))))


void util_init() {
  setvbuf(stdout, NULL, _IONBF, 0); 
  int err = MPI_Comm_set_errhandler(MPI_COMM_WORLD, MPI_ERRORS_ARE_FATAL);
  if (err) {
    exit(1);
  }
}


#endif
