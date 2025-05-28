CC = gcc

CFLAGS = -Wall -g -pedantic -std=gnu99 -Wextra -ggdb3

LIBDIR = /usr/lib64 
LDFLAGS = -L$(LIBDIR) -lopencv_core -lopencv_imgcodecs -lopencv_objdetect -lopencv_imgproc -pthread

TARGETS = uqfaceclient uqfacedetect

.DEFAULT_GOAL := all

.PHONY: clean

all: $(TARGETS)

uqfaceclient: uqfaceclient.c
	$(CC) $(CFLAGS) -o $@ $<

uqfacedetect: uqfacedetect.c
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $<

%: %.c
	$(CC) $(CFLAGS) -o $@ $<

clean:
	rm -f *.o
