#!/usr/bin/env bash
###
### run_tests.sh — run libmpidynres tests
###
### Usage:
###   run_tests.sh <builddir>
###
### Options:
###   <builddir> build directory from the make file

help() {
    sed -rn 's/^### ?//;T;p' "$0"
}

if [[ "$1" == "-h" ]]; then
    help
    exit 1
fi

RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[0;33m'
NC='\033[0m'

BUILDDIR="${1:-build}"



echo "LIBMPIDYNRES TESTS"
echo "==================="
NTESTS=$(ls -1 tests | sed -n '/^test_.*\.c/p' | sort | wc -l)
echo "Found $NTESTS tests to run"
echo
echo
echo


export HWLOC_DEBUG_VERBOSE=0
export MPIDYNRES_DEBUG=1 

failed=0
passed=0
invalid=0
ls -1 tests | sed -n '/^test_.*\.c/p' | sort | (while read testsrc; do
    testname="${testsrc%.*}"
    binary="./$BUILDDIR/tests/$testname"
    echo Source: tests/$testsrc
    echo Binary: $binary

    if [ ! -f "$binary" ]; then
        echo "It seems the test $testname wasn't build, please run \`make tests\` again"
        invalid="$(($invalid + 1))"
        continue
    fi
    if [ "tests/$testsrc" -nt "$binary" ]; then
        echo "It seems that the source file 'tests/$testsrc' is newer than the test itself, please run \`make tests\` again"
        invalid="$(($invalid + 1))"
        continue
    fi

    echo "Running test $testname..."

    if grep 'TEST_NEEDS_MPI' "tests/$testsrc" > /dev/null; then
        if grep 'TEST_MPI_RANKS' "tests/$testsrc" > /dev/null; then
            mpirun "$binary" < /dev/null
            RET=$?
        else
            mpirun "$binary"
            RET="$?"
        fi
    else
        ./$binary
        RET="$?"
    fi

    if [ "$RET" -eq "0" ]; then
        echo -e "$GREEN"Test "$testname" passed"$NC"
        passed="$(($passed + 1))"
    else
        echo -e "$RED"Test "$testname" failed with exit code "$RET""$NC"
        failed="$(($failed + 1))"
    fi
    echo 
    echo 
    echo 
    echo 
done


echo Summary
echo ==============
echo from $NTESTS tests:
if [ "$passed" -gt 0 ]; then
    echo -e "$GREEN"$passed tests passed"$NC"
fi

if [ "$failed" -gt 0 ]; then
    echo -e "$RED"$failed tests failed"$NC"
fi

if [ "$invalid" -gt 0 ]; then
    echo -e "$YELLOW" $invalid tests were invalid"$NC"
fi
)
