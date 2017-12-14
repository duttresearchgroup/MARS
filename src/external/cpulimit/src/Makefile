CC?=gcc
AR?=ar
CFLAGS?=-Wall -g -D_GNU_SOURCE
TARGETS=cpulimit
LIBS=libcpulimit.a

UNAME := $(shell uname)

ifeq ($(UNAME), FreeBSD)
LIBS+=-lkvm
endif

all::	$(TARGETS) $(LIBS)

cpulimit: main.o $(LIBS)
	$(CC) -static -o cpulimit main.o $(LIBS) $(CFLAGS)

libcpulimit.a: cpulimit.o process_iterator.o list.o process_group.o
	$(AR) rvs $@ cpulimit.o process_iterator.o list.o process_group.o

main.o: main.c cpulimit.h
	$(CC) -c main.c $(CFLAGS)

cpulimit.o: cpulimit.c cpulimit.h
	$(CC) -c cpulimit.c $(CFLAGS)

process_iterator.o: process_iterator.c process_iterator.h
	$(CC) -c process_iterator.c $(CFLAGS)

list.o: list.c list.h
	$(CC) -c list.c $(CFLAGS)

process_group.o: process_group.c process_group.h
	$(CC) -c process_group.c $(CFLAGS)

clean:
	rm -f cpulimit *~ *.o libcpulimit.a $(TARGETS)

