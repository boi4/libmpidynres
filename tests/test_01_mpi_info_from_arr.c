/*
 * TEST_NEEDS_MPI
 * TEST_MPI_RANKS 1
 */
#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>

#include "util_test.h"
#include "util_info.h"

#include "../src/mpidynres.h"


void run_test(size_t vec_len, char const * const vec[]) {
  static int n = 0;
  n++;

  if (vec_len % 2 != 0) {
    printf("Invalid veclen!\n");
    exit(1);
  }

  int err;
  MPI_Info info;

  err = MPIDYNRES_Info_create_strings(vec_len, vec, &info);
  if (err) {
    printf(
        "MPIDYNRES_Info_create_strings returned with non zero"
        "exit status %d in iteration %d\n",
        err, n);
    MPI_Finalize();
    exit(1);
  }
  compare_info_vec(vec_len, vec, info);

  MPI_Info_free(&info);
}


int main(int argc, char *argv[]) {
  MPI_Init(&argc, &argv);
  util_init();

  run_test(COUNT_OF(TEST_CASE_1), TEST_CASE_1);
  run_test(COUNT_OF(TEST_CASE_2), TEST_CASE_2);
  run_test(COUNT_OF(TEST_CASE_3), TEST_CASE_3);
  run_test(COUNT_OF(TEST_CASE_4), TEST_CASE_4);

  MPI_Finalize();
  return 0;
}
