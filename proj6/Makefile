CC   = gcc
OPTS = -g -Wall -pedantic

all: server client libmfs.so

# this generates the target executables
server: server.o udp.o
	$(CC) $(OPTS) -o $@ $^

client: client.o udp.o libmfs.so
	$(CC) $(OPTS) -L. -lmfs -o $@ $^

libmfs.so: mfs.o
	$(CC) $(OPTS) -shared -o $@ $^

mfs.o: mfs.c
	$(CC) $(OPTS) -fpic -c $< -o $@

# this is a generic rule for .o files
%.o: %.c
	$(CC) $(OPTS) -c $< -o $@

clean:
	rm -f libmfs.so server.o udp.o mfs.o client.o server client
