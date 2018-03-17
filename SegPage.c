#include "SegPage_t.h"

static char mem[ARRSIZE];

mb * front = NULL; // A pointer to the first allocation

/*  __createMeta()__
 *  - Fills in the metadata before a malloced chunk
 */
void * createMeta(void * start, int newSize, mb * newNextBlock){
  mb * placeData = (mb *) start;
  (*placeData).size = newSize;
  (*placeData).next = newNextBlock;

  return (void *) (placeData + 1);
}


/* __myallocate()__
 * - Blah...
 */
void * myallocate(int size, char *  file, int line, int type){
  if(size < 1 || size + METASIZE > ARRSIZE) {
		fprintf(stderr, "ERROR: Can't malloc < 0 or greater then %d byte - File: %s, Line: %d", (ARRSIZE - METASIZE), file, line);
    return NULL;
	}

  // Place as new front
	if(front == NULL) {
    front = (mb *) mem;
    return createMeta((void *) mem, size, NULL);
  }

  //Place before front
  if( ((char *) front) - mem >= size + METASIZE){
    mb * temp = front;
    front = (mb *) mem;
    return createMeta((void *) mem, size, temp);
  }

  // Place in the Middle
  mb * ptr = front;
  while((*ptr).next != NULL){

    // Put it in the space between
    if ( ((char *) (*ptr).next) - ( ((char *) ptr) + METASIZE + (*ptr).size) >= size + METASIZE) {
      mb * temp = (*ptr).next;
      (*ptr).next = (mb *) ( ((char *) ptr) + METASIZE + (*ptr).size);
      return createMeta((void *) (*ptr).next, size, temp);
    }

    ptr = (*ptr).next;
  }

  // Place at the end
  if ( (mem + ARRSIZE) - ( ((char *) ptr) + METASIZE + (*ptr).size) >= size + METASIZE) {
    (*ptr).next = (mb *) ( ((char *) ptr) + METASIZE + (*ptr).size);
    return createMeta((void *) (*ptr).next, size, NULL);
  }

  fprintf(stderr, "ERROR: Not enough space to malloc. - File: %s, Line: %d\n", file, line);
  return NULL;
}

/* __mydeallocate()__
 * - Blah...
 */
void mydeallocate(void * freeThis, char * file, int line, int type){
	if(front == NULL){
    fprintf(stderr, "ERROR: No malloced chunks to free - File: %s, Line: %d\n", file, line);
    return;

  } else if(freeThis == NULL){
    fprintf(stderr, "ERROR: Cannot free a NULL pointer - File: %s, Line: %d\n", file, line);
    return;
  }

  // First metaBlock
  if(freeThis == (void *)(front + 1)){
    front = (*front).next;
    return;
  }

  // Middle + End metaBlocks
  mb * ptr = front;
  while((*ptr).next != NULL){
    if(freeThis == (void *)((*ptr).next + 1)){
      (*ptr).next = (*((*ptr).next)).next;
      return;
    }

    ptr = (*ptr).next;
  }

  fprintf(stderr, "ERROR: Cannot free an unmalloced pointer - File: %s, Line: %d\n", file, line);
  return;
}
