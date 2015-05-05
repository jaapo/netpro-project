CC=gcc
CFLAGS=-std=c99 -ggdb -Wall -pedantic
BINS=$(addprefix bin/,protomsg.o fap.o dcp.o fctp.o fscli fsdird fsservd)

.PHONY: all
all: $(BINS)

bin/util.o: util.c util.h | bin
	$(CC) $(CFLAGS) -c -o bin/util.o util.c

bin/protomsg.o: protomsg.c protomsg.h | bin
	$(CC) $(CFLAGS) -c -o bin/protomsg.o protomsg.c

bin/dcp.o: dcp.c dcp.h bin/protomsg.o | bin
	$(CC) $(CFLAGS) -c -o bin/dcp.o dcp.c bin/protomsg.o

bin/fctp.o: fctp.c fctp.h bin/protomsg.o | bin
	$(CC) $(CFLAGS) -c -o bin/fctp.o fctp.c bin/protomsg.o

bin/fap.o: fap.c fap.h bin/protomsg.o | bin
	$(CC) $(CFLAGS) -c -o bin/fap.o fap.c bin/protomsg.o

bin/fscli: fscli.c fscli.h bin/fap.o bin/util.o | bin
	$(CC) $(CFLAGS) -o bin/fscli fscli.c bin/fap.o bin/util.o

bin/fsdird: fsdird.c fsdird.h bin/dcp.o | bin
	$(CC) $(CFLAGS) -o bin/fsdird fsdird.c bin/dcp.o

bin/fsservd: fsservd.c fsservd.h bin/dcp.o bin/fctp.o bin/fap.o | bin
	$(CC) $(CFLAGS) -o bin/fsservd fsservd.c bin/dcp.o bin/fctp.o bin/fap.o

bin:
	mkdir -p bin

.PHONY: clean
clean:
	rm -f bin/*