CC = gcc

CFLAGS = -Wall -g -pedantic -std=gnu99 -Wextra -pthread

LIBDIR = /usr/lib64 
LDFLAGS = -L$(LIBDIR) -lopencv_core -lopencv_imgcodecs -lopencv_objdetect -lopencv_imgproc 
CFLAGS += $(LDFLAGS)

TARGETS = uqfaceclient uqfacedetect

.DEFAULT_GOAL := all

.PHONY: clean

all:  $(TARGETS)

uqfaceclient: uqfaceclient.c

uqfacedetect: uqfacedetect.c

%: %.c
	$(CC) $(CFLAGS) -o $@ $<

clean:
	rm -f *.o
