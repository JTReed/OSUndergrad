CC=gcc
CFLAGS=-g

all : test

test.o : test.c
	$(CC) $(CFLAGS) -c test.c

mem.o : mem.c
	$(CC) $(CFLAGS) -c -fpic mem.c

libmem.so: mem.o
	$(CC) $(CFLAGS) -shared -o $@ $<

test : test.o libmem.so
	$(CC) $(CFLAGS) $< -o $@ -lmem -L.


clean :
	rm -rf *.o test test9 *.so
