CC=gcc
CFLAGS=-std=c99 -ggdb -Wall -pedantic -D_GNU_SOURCE
BINS=$(addprefix bin/,util.o protomsg.o fap.o dcp.o fctp.o fscli fsdird fsservd)

.PHONY: all
all: $(BINS)

bin/util.o: util.c util.h | bin
	$(CC) $(CFLAGS) -c -o bin/util.o util.c

bin/netutil.o: netutil.c netutil.h | bin
	$(CC) $(CFLAGS) -c -o bin/netutil.o netutil.c

bin/protomsg.o: protomsg.c protomsg.h | bin
	$(CC) $(CFLAGS) -c -o bin/protomsg.o protomsg.c

bin/dcp.o: dcp.c dcp.h | bin
	$(CC) $(CFLAGS) -c -o bin/dcp.o dcp.c

bin/fctp.o: fctp.c fctp.h bin/protomsg.o | bin
	$(CC) $(CFLAGS) -c -o bin/fctp.o fctp.c

bin/fap.o: fap.c fap.h bin/protomsg.o | bin
	$(CC) $(CFLAGS) -c -o bin/fap.o fap.c

bin/fscli: fscli.c fscli.h bin/fap.o bin/util.o bin/netutil.o | bin
	$(CC) $(CFLAGS) -o bin/fscli fscli.c bin/fap.o bin/util.o bin/netutil.o bin/protomsg.o

bin/fsdird: fsdird.c fsdird.h bin/dcp.o bin/util.o bin/netutil.o | bin
	$(CC) $(CFLAGS) -pthread -o bin/fsdird fsdird.c bin/dcp.o bin/util.o bin/protomsg.o bin/netutil.o

bin/fsservd: fsservd.c fsservd.h bin/dcp.o bin/fctp.o bin/fap.o bin/util.o bin/netutil.o | bin
	$(CC) $(CFLAGS) -pthread -o bin/fsservd fsservd.c bin/dcp.o bin/fctp.o bin/fap.o bin/protomsg.o bin/util.o bin/netutil.o

bin:
	mkdir -p bin

.PHONY: clean
clean:
	rm -f bin/*
