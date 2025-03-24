# Compiler
CC = gcc

# Compiler flags
CFLAGS = -Wall -pedantic -std=gnu99 -Wextra -g 

# Library
LIBDIR = /local/courses/csse2310/lib
LDFLAGS = -L$(LIBDIR) -ltinyexpr -lm

# include/dynamic-library-h
INCLUDEDIR = /local/courses/csse2310/include
CFLAGS += -I$(INCLUDEDIR)

# Default target
.DEFAULT_GOAL := uqexpr 

# Do not generate
.PHONY: clean

uqexpr: uqexpr.c

clean:
	rm -f uqexpr *.o
