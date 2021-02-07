/*
 * TEST_NEEDS_MPI
 * TEST_MPI_RANKS 2
 **/
#include <mpi.h>

#include "../src/comm.h"
#include "util_info.h"
#include "util_test.h"

enum {
  TAG1,
  TAG2,
};

void rank0(size_t vec_len, char const *const vec[]) {
  static int n = 0;
  n++;
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

  err = MPIDYNRES_Send_MPI_Info(info, 1, TAG1, TAG2, MPI_COMM_WORLD);
  if (err != 0) {
    printf("MPIDYNRES_Send_MPI_Info returned with an error (rank0)\n");
    MPI_Finalize();
    exit(1);
  }
  err = MPIDYNRES_Recv_MPI_Info(&info, 1, TAG1, TAG2, MPI_COMM_WORLD,
                                MPI_STATUS_IGNORE, MPI_STATUS_IGNORE);
  if (err != 0) {
    printf("MPIDYNRES_Recv_MPI_Info returned with an error (rank0)\n");
    MPI_Finalize();
    exit(1);
  }

  compare_info_vec(vec_len, vec, info);
}

void rank1() {
  int err;
  MPI_Info info;
  err = MPIDYNRES_Recv_MPI_Info(&info, 0, TAG1, TAG2, MPI_COMM_WORLD,
                                MPI_STATUS_IGNORE, MPI_STATUS_IGNORE);
  if (err != 0) {
    printf("MPIDYNRES_Recv_MPI_Info returned with an error (rank1)\n");
    MPI_Finalize();
    exit(1);
  }
  err = MPIDYNRES_Send_MPI_Info(info, 0, TAG1, TAG2, MPI_COMM_WORLD);
  if (err != 0) {
    printf("MPIDYNRES_Send_MPI_Info returned with an error (rank1)\n");
    MPI_Finalize();
    exit(1);
  }
}

void run_test(size_t vec_len, char const *const vec[]) {
  int rank, size;
  MPI_Comm_size(MPI_COMM_WORLD, &size);
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);

  switch (rank) {
    case 0: {
      rank0(vec_len, vec);
      break;
    }
    case 1: {
      rank1();
      break;
    }
  }
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
