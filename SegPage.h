#include <stdio.h>
#include <stdlib.h>
#include <string.h>

//Through factory.cs.rutgers.edu

#ifndef _SEGPAGE_H_
#define _SEGPAGE_H_

#define THREADREQ 0
#define LIBRARYREQ 1

#define malloc(x) myallocate(x, __FILE__, __LINE__, THREADREQ) //x is a size here
#define free(x) mydeallocate(x, __FILE__, __LINE__, THREADREQ) //HEY FUCKING LISTEN, x IS NOT A SIZE

#define ARRSIZE 1024*1024*8

/*Structure for Malloc*/
typedef struct metaBlock {
  void * nextBlock;
  int size;
  int loc;
  // int free;
} mb;

#define METASIZE sizeof(struct metaBlock)
//So far, the metadata size is 16 in factory.cs.rutgers.edu

// ____createMeta____
void * createMeta(int start, int newSize, mb * newNextBlock);

// ____myallocate____
void * myallocate(int size, char *  file, int line, int type);

// ____mydeallocate____
void * mydeallocate(void * freeThis, char * file, int line, int type);

#endif /* _SEGPAGE_H_ */
