#include "SegPage_t.h"

char * mem = NULL;
pageInfo * tableFront = NULL;

mb * libFront = NULL; // A pointer to the first allocation

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
void * t_myallocate(int size, char *  file, int line, char * memStart, int memSize, mb ** frontPtr){
  if(size < 1 || size + METASIZE > memSize) {
		fprintf(stderr, "ERROR: Can't malloc < 0 or greater then %d byte - File: %s, Line: %d", (memSize - METASIZE), file, line);
    return NULL;
	}

  // Place as new front
	if((*frontPtr) == NULL) {
    (*frontPtr) = (mb *) memStart;
    return createMeta((void *) memStart, size, NULL);
  }

  //Place before front
  if( ((char *) (*frontPtr)) - memStart >= size + METASIZE){
    mb * temp = (*frontPtr);
    (*frontPtr) = (mb *) memStart;
    return createMeta((void *) memStart, size, temp);
  }

  // Place in the Middle
  mb * ptr = (*frontPtr);
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
  if ( (memStart + memSize) - ( ((char *) ptr) + METASIZE + (*ptr).size) >= size + METASIZE) {
    (*ptr).next = (mb *) ( ((char *) ptr) + METASIZE + (*ptr).size);
    return createMeta((void *) (*ptr).next, size, NULL);
  }

  fprintf(stderr, "ERROR: Not enough space to malloc. - File: %s, Line: %d\n", file, line);
  return NULL;
}

void * myallocate(int size, char *  file, int line, int type){
  struct itimerval res = disableTimer(); // PAUSE TIMER

  uint tid;
  mb ** currFront;

  // Who called myallocate()?
  if(type == LIBRARYREQ){ // Request from the scheduler
    tid = 1;
    currFront = &libFront;

  } else { //Its a thread
    if(currCtxt == NULL){ // Threads have not been created yet
      // Create a thread for main
      struct sigaction sigAct;
  		sigAct.sa_handler = scheduler;
  		sigAct.sa_flags = 0;
  		sigaction(T_SIG, &sigAct, NULL);

  		currCtxt = myallocate(sizeof(tcbNode), __FILE__, __LINE__, LIBRARYREQ);
  		if(currCtxt == NULL){
  			//ERROR
  			return NULL;
  		}
  		(*currCtxt).next = NULL;

  		// Set defaults
  		(*currCtxt).data.tID = __sync_fetch_and_add(&idCount, 1); //Fetch and increment idCounter atomically
  		(*currCtxt).data.stat = P_RUN;
  		(*currCtxt).data.qNum = 1;
  		(*currCtxt).data.ret = NULL;
  		(*currCtxt).data.w_mutex = NULL;
  		(*currCtxt).data.w_tID = 0;
  		(*currCtxt).data.front = NULL;

  		// Create ucontext
  		getcontext(&(*currCtxt).data.ctxt);
    }

    // Set variables for rest of method
    tid = (*currCtxt).data.tID;
    currFront = &(*currCtxt).data.front;
  }

  // Has memory been created yet?
  if(mem == NULL){
    mem = (char *) memalign(PAGE_SIZE, 1024*1024*8);
    if(mem == NULL){
      //ERROR
      return NULL;
    }

    // Clear Page table space + set table front
    tableFront = (pageInfo *) ((char *)(mem + PAGE_SIZE*255));

    pageInfo * temp = tableFront;
    int i = 0;
    for(i = 0; i < 255; i++){
      (*temp).tid = 0;
      (*temp).index = 0;
      temp = temp + 1;
    }
  }

  // How much space does this thread have? + Find 1st empty page + Find num of free pages + thread's first page
  int numPages = 0;
  int numFreePages = 0;
  pageInfo * firstEmpty = NULL;
  pageInfo * firstPage = NULL;

  pageInfo * temp = tableFront;
  int i = 0;
  for(i = 0; i < 255; i++){
    if((*temp).tid == tid) {
      if((*temp).index == 0)
        firstPage = temp;
      numPages += 1;
    }

    if((*temp).tid == 0){
      if(firstEmpty == NULL)
        firstEmpty = temp;
      numFreePages += 1;
    }
    temp = temp + 1;
  }

  // If it had no Pages
  if(numPages == 0){
    (*firstEmpty).tid = tid;
    (*firstEmpty).index = 0;

    firstPage = firstEmpty;
  }

  // Find Page in memory
  char * pageLoc = mem + (firstPage - tableFront)*PAGE_SIZE;

  // Try to malloc there
  void * ret = t_myallocate(size, file, line, pageLoc, PAGE_SIZE, currFront);

  setitimer(WHICH, &res, NULL); // RESUME TIMER
  return ret;
}


/* __mydeallocate()__
 * - Blah...
 */
void t_mydeallocate(void * freeThis, char * file, int line, mb ** frontPtr){
	if((*frontPtr) == NULL){
    fprintf(stderr, "ERROR: No malloced chunks to free - File: %s, Line: %d\n", file, line);
    return;

  } else if(freeThis == NULL){
    fprintf(stderr, "ERROR: Cannot free a NULL pointer - File: %s, Line: %d\n", file, line);
    return;
  }

  // First metaBlock
  if(freeThis == (void *)((*frontPtr) + 1)){
    (*frontPtr) = (*(*frontPtr)).next;
    return;
  }

  // Middle + End metaBlocks
  mb * ptr = (*frontPtr);
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

void mydeallocate(void * freeThis, char * file, int line, int type){
  struct itimerval res = disableTimer(); // PAUSE TIMER

  uint tid;
  mb ** currFront;

  // Who called mydeallocate()?
  if(type == LIBRARYREQ){ // Request from the scheduler
    tid = 1;
    currFront = &libFront;

  } else { //Its a thread
    if(currCtxt == NULL){ // Threads have not been created yet
      fprintf(stderr, "ERROR: Can't free un-malloced space - File: %s, Line: %d", file, line);
      return;
    }

    // Set variables for rest of method
    tid = (*currCtxt).data.tID;
    currFront = &(*currCtxt).data.front;
  }

  // Leave error if memory hasnt been created yet
  if(mem == NULL){
    fprintf(stderr, "ERROR: Can't free un-malloced space - File: %s, Line: %d", file, line);
    return;
  }

  // Deallocate the given pointer if possible
  t_mydeallocate(freeThis, file, line, currFront);

  setitimer(WHICH, &res, NULL); // RESUME TIMER
}
