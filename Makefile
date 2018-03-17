CC = gcc
CFLAGS = -g -c
LDFLAGS =-lmy_pthread
AR = ar -rc
RANLIB = ranlib

Target: tester

SegPage.a: SegPage.o
	$(AR) libSegPage.a SegPage.o
	$(RANLIB) libSegPage.a

SegPage.o: SegPage_t.h
	$(CC) $(CFLAGS) SegPage.c

tester: SegPage.o
	$(CC) tester.c SegPage.o

clean:
	rm -rf *.o *.a *.out
