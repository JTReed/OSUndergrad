#
# To compile, type "make" or make "all"
# To remove files, type "make clean"
#
OBJS = server.o request.o cs537.o client.o
TARGET = server

CC = gcc
CFLAGS = -g -Wall -pedantic

LIBS = -lpthread

.SUFFIXES: .c .o

all: server client output.cgi

server: server.o request.o cs537.o
	$(CC) $(CFLAGS) -o server server.o request.o cs537.o $(LIBS)

client: client.o cs537.o
	$(CC) $(CFLAGS) -o client client.o cs537.o

output.cgi: output.c
	$(CC) $(CFLAGS) -o output.cgi output.c

.c.o:
	$(CC) $(CFLAGS) -o $@ -c $<

clean:
	-rm -f $(OBJS) server client output.cgi

# Local variables:
# c-basic-offset: 8
# tab-width: 8
# indent-tabs-mode: t
# End:
#
# vi: set shiftwidth=8 tabstop=8 noexpandtab:
# :indentSize=8:tabSize=8:noTabs=false:
