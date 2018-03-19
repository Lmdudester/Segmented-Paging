//Through factory.cs.rutgers.edu

#ifndef _SEGPAGE_H_
#define _SEGPAGE_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>
#include <sys/mman.h>

#define THREADREQ 1
#define LIBRARYREQ 0

#define malloc(x) myallocate(x, __FILE__, __LINE__, THREADREQ) //x is a size
#define free(x) mydeallocate(x, __FILE__, __LINE__, THREADREQ) //x is a pointer

#define ARRSIZE 1024*1024*8

#define PAGE_SIZE sysconf( _SC_PAGE_SIZE)

#define TOTAL_PAGES 2048
#define THREAD_PAGES 1024
#define TABLE_PAGES 4
#define LIB_PAGES 1020

#define METASIZE sizeof(struct metaBlock)
//So far, the metadata size is 16 in factory.cs.rutgers.edu

char isLib;

/*Structure for Malloc*/
typedef struct metaBlock {
  struct metaBlock * next;
  int size;
} mb;

#include "my_pthread_t.h"

typedef struct pageInfo {
  uint tid;
  uint index;
  mb * front;
} pageInfo;

void printPT(int howMany);

// ____myallocate____
void * myallocate(int size, char *  file, int line, int type);

// ____mydeallocate____
void mydeallocate(void * freeThis, char * file, int line, int type);

#endif /* _SEGPAGE_H_ */
