BINS=directory_walker
CC=clang
CFLAGS=-Wall
IFLAGS=-I../src/minix/fs

all: $(BINS)

%: %.c
	$(CC) -o $@ $< $(CFLAGS) $(IFLAGS)

clean:
	rm $(BINS)

.PHONY: all clean
