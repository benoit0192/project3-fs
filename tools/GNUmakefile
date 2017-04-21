BINS=directory_walker inode_bitmap_walker zone_bitmap_walker
CC=clang
CFLAGS=-Wall
IFLAGS=-I../src/minix/fs

all: $(BINS)

%: %.c
	$(CC) -o $@ $< $(CFLAGS) $(IFLAGS)

clean:
	rm $(BINS)

.PHONY: all clean
