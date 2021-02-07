#ifndef UTIL_INFO_H
#define UTIL_INFO_H

#include <string.h>
#include <stdlib.h>
#include <stdio.h>


char const *const TEST_CASE_1[] = {
    "testkey", "testval",
    "asdf",    "asdfjlkas",
};

char const *const TEST_CASE_2[] = {
    "testkey2",         "testval2",
    "tesasd",           "tesvasdvval2",
    "tevjwoijstkey2",   "testl2",
    "testasdvjwekey2",  "tesdilksda;jval2",
    "testisdljvakey2",  "tesaksdfjvas;39028tval2",
    "testjasmmkey2",    "teaslkestval2",
    "testkeyasjdfme32", "testvvamslk3al2",
    "teaml3kmstkey2",   "teaml3klstval2",
    "testkeaml3y2",     "test3mlafval2",
    "testk3mey2",       "tes3mlatval2",
};

char const *const TEST_CASE_3[] = {
    "asdf", "asdf",
};

char const *const TEST_CASE_4[] = {};


void compare_info_vec(size_t vec_len, char const * const vec[], MPI_Info info) {
  int nkeys;
  MPI_Info_get_nkeys(info, &nkeys);
  if (nkeys != vec_len/2) {
    printf("Invalid number of keys in info object, expected %zu, got %d\n", vec_len/2, nkeys);
    MPI_Finalize();
    exit(1);
  }

  for (size_t i = 0; i < vec_len; i+=2) {
    int is_inside;
    int vlen;
    char const *key = vec[i];

    MPI_Info_get_valuelen(info, key, &vlen, &is_inside);
    if (!is_inside) {
      printf("Info doesn't contain %s\n", key);
      exit(1);
    }
    char *val = calloc(1, vlen + 1);
    if (!val) {
      printf("Memory error\n");
      exit(1);
    }
    MPI_Info_get(info, key, vlen, val, &is_inside);
    if (strcmp(val, vec[i+1])) {
      printf("Value mismatch with key %s: Expected %s, got %s\n", key, vec[i+1], val);
      exit(1);
    }
    free(val);
  }

  MPI_Info_free(&info);
}

#endif
