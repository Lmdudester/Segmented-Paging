// File:	SegPage_t.h
// Authors: Liam Davies, Kevin Lee, Brian Ellsworth
// username of iLab: lmd312, kjl156, bje40
// iLab Server: factory.cs.rutgers.edu

#ifndef _SEGPAGE_H_
#define _SEGPAGE_H_

/* Imports */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

/* Request Permission Macros */
#define THREADREQ 1
#define LIBRARYREQ 0

/* Library Transformation Macros */
#define malloc(x) myallocate(x, __FILE__, __LINE__, THREADREQ) //x is a size
#define free(x) mydeallocate(x, __FILE__, __LINE__, THREADREQ) //x is a pointer
#define shalloc(x) myshalloc(x, __FILE__, __LINE__) //x is a size

/* Memory-Space/Page-Sizing Macros */
#define ARRSIZE 1024*1024*8

#define PAGE_SIZE sysconf( _SC_PAGE_SIZE)

#define TOTAL_PAGES 2048
#define TOTAL_FILE_PAGES 4096

#define THREAD_PAGES 1020
#define SHARED_PAGES 4

#define M_TABLE_PAGES 2
#define F_TABLE_PAGES 8
#define LIB_PAGES 1014

/* Struct + Macro for internal metadata for Malloc */
#define METASIZE sizeof(struct metaBlock) // 16 on factory.cs.rutgers.edu

typedef struct metaBlock {
  struct metaBlock * next;
  uint size;
} mb;

/* Connect to pthread */
#include "my_pthread_t.h" // Have to do this here to allow access to above macros/structs

/* Struct for page table design */
typedef struct pageInfo {
  uint tid;
  uint index;
} pageInfo;


// ____myallocate____
//    Call with - malloc(x)
void * myallocate(size_t size, char *  file, int line, int type);

// ____mydeallocate____
//    Call with - free(x)
void mydeallocate(void * freeThis, char * file, int line, int type);

// ____shalloc____
//    Call with shalloc(x)
void * myshalloc(size_t size, char *  file, int line);

#endif /* _SEGPAGE_H_ */
