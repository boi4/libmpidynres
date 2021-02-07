.PHONY: clean, all, example, run_example, install, build, docs, upload, examples, buildremote, tests, test

BUILD_DIR ?= build

SRC_DIR ?= src

EXAMPLE_DIR ?= example

OBJ_DIR ?= $(BUILD_DIR)/tmp

LIB_DIR = $(BUILD_DIR)/lib

INSTALL_PREFIX = /usr/local/


MPICC ?= mpicc

CFLAGS ?= -fPIC -Wall -Wpedantic -Wextra -Werror=implicit-function-declaration -Werror=format-security \
					-ggdb -O0 \
					-I $(BUILD_DIR)/include

LDFLAGS ?= -L $(BUILD_DIR)/lib -lm



INCLUDE_EXPORT = $(wildcard public/*.h)
INCLUDE_EXPORT_DIR = $(BUILD_DIR)/include
INCLUDE_EXPORT_FILES = $(subst public,$(INCLUDE_EXPORT_DIR),$(INCLUDE_EXPORT))

SRCS = $(shell find $(SRC_DIR) -name *.c)
OBJS_TMP = $(SRCS:.c=.o)
OBJS = $(subst $(SRC_DIR),$(OBJ_DIR),$(OBJS_TMP))
TESTS_TMP = $(shell ls -1 tests/test_*.c)
TESTS_TMP2 = $(TESTS_TMP:.c=)
TESTS = $(subst tests/,$(BUILD_DIR)/tests/,$(TESTS_TMP2))


all: build

build: $(LIB_DIR)/libmpidynres.so $(INCLUDE_EXPORT_FILES)

$(OBJS): $(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	mkdir -p $(dir $@)
	$(MPICC) $(CFLAGS) -c $^ -o $@

$(LIB_DIR)/libmpidynres.so: $(OBJS)
	mkdir -p $(LIB_DIR)
	$(MPICC) ${LDFLAGS} --shared -o $@ $^

$(INCLUDE_EXPORT_FILES): $(INCLUDE_EXPORT)
	mkdir -p $(INCLUDE_EXPORT_DIR)
	cp -L $(INCLUDE_EXPORT) $(INCLUDE_EXPORT_DIR)

$(TESTS): $(BUILD_DIR)/tests/%: tests/%.c $(OBJS)
	mkdir -p "$(BUILD_DIR)/tests"
	$(MPICC) ${LDFLAGS} $^ -o $@

tests: $(TESTS)

test: tests
	bash ./run_tests.sh "$(BUILD_DIR)"

install: build
	rsync --exclude 'tmp' --exclude 'tests' -avP $(BUILD_DIR)/ $(INSTALL_PREFIX)/

docs: $(SRCS) docs/Doxyfile
	cd docs; doxygen

clean:
	rm -rf $(BUILD_DIR)

examples: build
	make -C example

buildremote: upload
	ssh lxlogin4.lrz.de '   \
MPICC=mpiicc make -C ~/libmpidynres examples'

upload:
	rsync --exclude build/\* --include example/\*.c --include example/Makefile --exclude example/\* -avz ./ lxlogin4.lrz.de:libmpidynres/
	rsync -avz ./../util/ lxlogin4.lrz.de:util/
