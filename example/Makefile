.PHONY: all, examples, clean

SRC_DIR ?= .

SRCS = $(shell find $(SRC_DIR) -name '*.c')

TGTS = $(SRCS:.c=)

MPICC ?= mpicc

CFLAGS ?= -Wall -Wpedantic -Wextra -Werror=implicit-function-declaration -Werror=format-security -ggdb \
					-I ../build/include -L ../build/lib

all: examples

examples: $(TGTS)

%: %.c
	$(MPICC) $(CFLAGS) $< -o $@ -lmpidynres

clean:
	rm -f $(TGTS)
