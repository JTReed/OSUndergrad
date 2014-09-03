CC = gcc
CFLAGS += -O1 -ffast-math -fomit-frame-pointer -Wall -pedantic

all: libspin.so libcounter.so liblist.so libhash.so

test: test.o libspin.so libcounter.so liblist.so libhash.so
	$(CC) $(CFLAGS) $^ -lpthread -o $@

counter_incr_bench: counter_incr_bench.o libspin.so libcounter.so
	$(CC) $(CFLAGS) $^ -lpthread -lm -o $@

list_insert_bench: list_insert_bench.o libspin.so liblist.so libhash.so
	$(CC) $(CFLAGS) $^ -lpthread -lm -o $@

hash_insert_bench: hash_insert_bench.o libspin.so liblist.so libhash.so
	$(CC) $(CFLAGS) $^ -lpthread -lm -o $@

hash_scale_bench: hash_scale_bench.o libspin.so liblist.so libhash.so
	$(CC) $(CFLAGS) $^ -lpthread -lm -o $@

libspin.so: spin.o
	$(CC) $(CFLAGS) -shared $^ -o $@

libcounter.so: counter.o
	$(CC) $(CFLAGS) -shared $^ -o $@

liblist.so: list.o
	$(CC) $(CFLAGS) -shared $^ -o $@

libhash.so: hash.o
	$(CC) $(CFLAGS) -shared $^ -o $@

test.o: test.c
	$(CC) $(CFLAGS) -fPIC -c $^ -o $@

counter_incr_bench.o: counter_incr_bench.c
	$(CC) $(CFLAGS) $^ -fPIC -c -o $@

list_insert_bench.o: list_insert_bench.c
	$(CC) $(CFLAGS) $^ -fPIC -c -o $@

hash_insert_bench.o: hash_insert_bench.c
	$(CC) $(CFLAGS) $^ -fPIC -c -o $@

hash_scale_bench.o: hash_scale_bench.c
	$(CC) $(CFLAGS) $^ -fPIC -c -o $@

spin.o: spin.c
	$(CC) $(CFLAGS) -fPIC -c $^ -o $@

counter.o: counter.c
	$(CC) $(CFLAGS) -fPIC -c $^ -o $@

list.o: list.c
	$(CC) $(CFLAGS) -fPIC -c $^ -o $@

hash.o: hash.c
	$(CC) $(CFLAGS) -fPIC -c $^ -o $@

.PHONY: clean distclean benchmark

benchmark:
	$(MAKE) clean
	$(MAKE)	counter_incr_bench \
		list_insert_bench \
		hash_insert_bench \
		hash_scale_bench
	LD_LIBRARY_PATH=. ./counter_incr_bench > counter_incr.spin
	LD_LIBRARY_PATH=. ./list_insert_bench > list_insert.spin
	LD_LIBRARY_PATH=. ./hash_insert_bench > hash_insert.spin
	LD_LIBRARY_PATH=. ./hash_scale_bench > hash_scale.spin
	$(MAKE) clean
	# leave a space in case user-defined CFLAGS is not empty
	CFLAGS+=\ -DPOSIX_MUTEX $(MAKE) counter_incr_bench \
					list_insert_bench \
					hash_insert_bench \
					hash_scale_bench
	LD_LIBRARY_PATH=. ./counter_incr_bench > counter_incr.posix
	LD_LIBRARY_PATH=. ./list_insert_bench > list_insert.posix
	LD_LIBRARY_PATH=. ./hash_insert_bench > hash_insert.posix
	LD_LIBRARY_PATH=. ./hash_scale_bench > hash_scale.posix
	gnuplot plot.gp
	$(MAKE) clean

distclean:
	$(MAKE) clean
	rm -fv	counter_incr.spin counter_incr.posix counter_incr.svg \
		list_insert.spin list_insert.posix list_insert.svg \
		hash_insert.spin hash_insert.posix hash_insert.svg \
		hash_scale.spin hash_scale.posix hash_scale.svg

clean:
	rm -fv	libspin.so libcounter.so liblist.so libhash.so spin.o \
		counter.o list.o hash.o test.o counter_incr_bench.o \
		list_insert_bench.o hash_insert_bench.o  hash_scale_bench.o \
		counter_incr_bench list_insert_bench hash_insert_bench \
		hash_scale_bench test

# Local variables:
# c-basic-offset: 8
# tab-width: 8
# indent-tabs-mode: t
# End:
#
# vi: set shiftwidth=8 tabstop=8 noexpandtab:
# :indentSize=8:tabSize=8:noTabs=false:
