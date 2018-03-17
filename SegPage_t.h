//Through factory.cs.rutgers.edu

#ifndef _SEGPAGE_H_
#define _SEGPAGE_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define THREADREQ 0
#define LIBRARYREQ 1

#define malloc(x) myallocate(x, __FILE__, __LINE__, THREADREQ) //x is a size
#define free(x) mydeallocate(x, __FILE__, __LINE__, THREADREQ) //x is a pointer

#define ARRSIZE 1024*1024*8

#define METASIZE sizeof(struct metaBlock)
//So far, the metadata size is 16 in factory.cs.rutgers.edu


/*Structure for Malloc*/
typedef struct metaBlock {
  struct metaBlock * next;
  int size;
} mb;

mb * front;

// ____myallocate____
void * myallocate(int size, char *  file, int line, int type);

// ____mydeallocate____
void mydeallocate(void * freeThis, char * file, int line, int type);

#endif /* _SEGPAGE_H_ */
