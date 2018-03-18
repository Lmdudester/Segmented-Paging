// File:	my_pthread_t.h
// Author:	Yujie REN
// Date:	09/23/2017

// name: Liam Davies, Kevin Lee, Brian Ellsworth
// username of iLab: lmd312, kjl156, bje40
// iLab Server: factory

#ifndef MY_PTHREAD_T_H
#define MY_PTHREAD_T_H

#define _GNU_SOURCE

/* include lib header files that you need here: */
#include <unistd.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <ucontext.h>
#include <sys/time.h>

#define USE_MY_PTHREAD //(comment this line if you want to use real pthread)

#ifdef USE_MY_PTHREAD
#define pthread_t my_pthread_t
#define pthread_mutex_t my_pthread_mutex_t
#define pthread_create my_pthread_create
#define pthread_yield my_pthread_yield
#define pthread_exit my_pthread_exit
#define pthread_join my_pthread_join
#define pthread_mutex_init my_pthread_mutex_init
#define pthread_mutex_lock my_pthread_mutex_lock
#define pthread_mutex_unlock my_pthread_mutex_unlock
#define pthread_mutex_destroy my_pthread_mutex_destroy
#endif

typedef enum State_
{
	P_RUN,
	P_YIELD,
	P_WAIT_M,
	P_WAIT_T,
	P_EXIT
} State;

#define WHICH ITIMER_REAL
#define	T_SIG SIGALRM

#define STKSZE 1024*64
#define MAINTAIN 10

#define Q1_MSECS 25
#define Q2_MSECS 50
#define Q3_MSECS 100

typedef uint my_pthread_t;

typedef union voidToint {
   int i;
   void * ptr;
} vToi;

// mutex struct definition
typedef struct my_pthread_mutex_t {
	int lock; // 0 - unlocked, 1 - locked, -1 - destroyed
	my_pthread_t tID; // thread that last locked
} my_pthread_mutex_t;

// tcb struct definition
typedef struct threadControlBlock {
  my_pthread_t tID; // Identification for the thread
  ucontext_t ctxt; // Contains all context info for thread
  my_pthread_mutex_t * w_mutex; // The mutex this thread is waiting on
  my_pthread_t w_tID; // The tid of the thread that this thread is waiting on (NOTE: 0 if none)
  int qNum; // The number of the prev queue this thread was in
  State stat; // Current status of the thread
  void * ret; // Pointer to return value
} tcb;

// tcb Node definition
typedef struct threadControlBlockNode {
  tcb data;
  struct threadControlBlockNode * next;
} tcbNode;

// Queues
tcbNode * q1;
tcbNode * q2;
tcbNode * q3;

// Linked Lists
tcbNode * finished;

// The currently running context
tcbNode * currCtxt;

// For threadID generation
my_pthread_t idCount;

// For Maintenance Cycle decisions
int numMaintain;

/* Function Declarations: */

/* create a new thread */
int my_pthread_create(my_pthread_t * thread, pthread_attr_t * attr, void *(*function)(void*), void * arg);

/* give CPU pocession to other user level threads voluntarily */
int my_pthread_yield();

/* terminate a thread */
void my_pthread_exit(void *value_ptr);

/* wait for thread termination */
int my_pthread_join(my_pthread_t thread, void **value_ptr);

/* initial the mutex lock */
int my_pthread_mutex_init(my_pthread_mutex_t *mutex, const pthread_mutexattr_t *mutexattr);

/* aquire the mutex lock */
int my_pthread_mutex_lock(my_pthread_mutex_t *mutex);

/* release the mutex lock */
int my_pthread_mutex_unlock(my_pthread_mutex_t *mutex);

/* destroy the mutex */
int my_pthread_mutex_destroy(my_pthread_mutex_t *mutex);

#endif
