CC=gcc

CFLAGS = -Wall -g -pedantic -std=gnu99 -Wextra

LIBDIR = /local/courses/csse2310/lib
LDFLAGS = -L$(LIBDIR) -lcsse2310a3

INCLUDEDIR = /local/courses/csse2310/include
CFLAGS += -I$(INCLUDEDIR)

.DEFAULT_GOAL := uqparallel

.PHONY: clean

uqparallel: uqparallel.c

%: %.c
	$(CC) $(CFLAGS) -o $@ $<

clean:
	rm -f *.o
