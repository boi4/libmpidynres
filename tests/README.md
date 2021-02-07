# Tests

All C files in this directory beginning with `test\_` are treated as single tests and have to contain a main function.

If the test passed, the program shall return with 0.

If the test needs mpi to run, it should contain the text `TEST_NEEDS_MPI` in its source code.

To specify the number of ranks in mpi comm world, it should contain the text `TEST_MPI_RANKS <N>`.

Tests are ran in alphabetical order.
