CC = gcc
CFLAGS = -g -c
LDFLAGS =-lmy_pthread -lSegPage
AR = ar -rc
RANLIB = ranlib

Target: all

all: Test1 Test2

# Seg-Mem
SegPage.a: SegPage.o
	$(AR) libSegPage.a SegPage.o
	$(RANLIB) libSegPage.a

SegPage.o: SegPage_t.h
	$(CC) $(CFLAGS) SegPage.c


# my_pthread
my_pthread.a: my_pthread.o
	$(AR) libmy_pthread.a my_pthread.o
	$(RANLIB) libmy_pthread.a

my_pthread.o: my_pthread_t.h
	$(CC) -pthread $(CFLAGS) my_pthread.c

pthreadTester: my_pthread.o
	$(CC) p_thread_test.c my_pthread.o


# Tests
Test1: ./Tests/Test1.c my_pthread.a SegPage.a
	$(CC) ./Tests/Test1.c -L. $(LDFLAGS) -o Test1

Test2: ./Tests/Test2.c my_pthread.a SegPage.a
	$(CC) ./Tests/Test2.c -L. $(LDFLAGS) -o Test2

Test3: ./Tests/Test3.c my_pthread.a SegPage.a
	$(CC) ./Tests/Test3.c -L. $(LDFLAGS) -o Test3

Test4: ./Tests/Test4.c my_pthread.a SegPage.a
	$(CC) ./Tests/Test4.c -L. $(LDFLAGS) -o Test4

practice: practice.c my_pthread.a SegPage.a
	$(CC) practice.c -L. $(LDFLAGS) -o practice


# Old_Tests
Old_Test1: ./Old_Tests/Test1.c my_pthread.a SegPage.a
	$(CC) ./Old_Tests/Test1.c -L. $(LDFLAGS) -o Old_Test1

Old_Test2: ./Old_Tests/Test2.c my_pthread.a
	$(CC) ./Old_Tests/Test2.c -L. $(LDFLAGS) -o Old_Test2

Old_Test3: ./Old_Tests/Test3.c my_pthread.a
	$(CC) ./Old_Tests/Test3.c -L. $(LDFLAGS) -o Old_Test3

Old_Test4: ./Old_Tests/Test4.c my_pthread.a
	$(CC) ./Old_Tests/Test4.c -L. $(LDFLAGS) -o Old_Test4

Old_Test5: ./Old_Tests/Test5.c my_pthread.a
	$(CC) ./Old_Tests/Test5.c -L. $(LDFLAGS) -o Old_Test5

Old_Test6: ./Old_Tests/Test6.c my_pthread.a
	$(CC) ./Old_Tests/Test6.c -L. $(LDFLAGS) -o Old_Test6

Old_Test7: ./Old_Tests/Test7.c my_pthread.a
	$(CC) ./Old_Tests/Test7.c -L. $(LDFLAGS) -o Old_Test7

Old_Test8: ./Old_Tests/Test8.c my_pthread.a
	$(CC) ./Old_Tests/Test8.c -L. $(LDFLAGS) -o Old_Test8


# Cleanup
clean:
	rm -rf Old_Test[0-9]* Test[0-9]* *.o *.a *.out mem.dat
