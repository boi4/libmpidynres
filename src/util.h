#ifndef UTIL_H
#define UTIL_H

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <signal.h>

// https://stackoverflow.com/questions/40807833/sending-size-t-type-data-with-mpi
#if SIZE_MAX == UCHAR_MAX
#define my_MPI_SIZE_T MPI_UNSIGNED_CHAR
#elif SIZE_MAX == USHRT_MAX
#define my_MPI_SIZE_T MPI_UNSIGNED_SHORT
#elif SIZE_MAX == UINT_MAX
#define my_MPI_SIZE_T MPI_UNSIGNED
#elif SIZE_MAX == ULONG_MAX
#define my_MPI_SIZE_T MPI_UNSIGNED_LONG
#elif SIZE_MAX == ULLONG_MAX
#define my_MPI_SIZE_T MPI_UNSIGNED_LONG_LONG
#else
#error "what is happening here?"
#endif


#define STR_TMP(x) #x
#define STR(x) STR_TMP(x)

//#define BREAK() raise(SIGTRAP)
#define BREAK() __asm__("int $3")

// https://stackoverflow.com/questions/4415524/common-array-length-macro-for-c
#define COUNT_OF(x) \
  ((sizeof(x) / sizeof(0 [x])) / ((size_t)(!(sizeof(x) % sizeof(0 [x])))))

#define die(fmt, ...)                                                    \
  do {                                                                   \
    fprintf(stderr, __FILE__ " line %d: " fmt, __LINE__, ##__VA_ARGS__); \
    BREAK();                                                             \
    exit(EXIT_FAILURE);                                                  \
  } while (0)

#endif
