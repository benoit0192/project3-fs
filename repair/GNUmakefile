CC=clang
CFLAGS=-Wall -I../src/minix/fs
SRCS=utils.c devio.c check.c

all: recovery damage

recovery: recovery.c $(SRCS)
	$(CC) -o $@ $^ $(CFLAGS)

damage: damage.c $(SRCS)
	$(CC) -o $@ $^ $(CFLAGS)

clean:
	rm recovery damage

.PHONY: all clean
