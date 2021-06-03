CC=gcc
CFLAGS=-Wall $(shell pkg-config --cflags zlib)
LDLIBS=$(shell pkg-config --libs zlib)

all: untuf

untuf:

clean:
	rm -f *.o untuf

.PHONY: clean

