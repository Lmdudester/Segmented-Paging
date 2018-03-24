#include "SegPage_t.h"

char * mem = NULL;
char * s_mem = NULL;
char * l_mem = NULL;

pageInfo * m_front = NULL;
pageInfo * f_front = NULL;

mb * libFront = NULL;

/* FOR TESTING */
void printPT(int howMany){
  if(m_front == NULL)
    return;
  pageInfo * ptr = m_front;
  int i = 0;
  for(i = 0; i < howMany; i++){
    printf("i: %d - tid: %d, index - %d\n", i, (*ptr).tid, (*ptr).index);
    ptr += 1;
  }
  printf("\n");
}

/* HELPERS */

void removePages(uint tid, pageInfo * front){
  pageInfo * ptr = m_front;

  int i = 0;
  for(i = 0; i < THREAD_PAGES; i++){
    if((*ptr).tid == tid){
      (*ptr).tid = 0;
      (*ptr).index = 0;
    }
    ptr += 1;
  }

  ptr = f_front;
  for(i = 0; i < TOTAL_FILE_PAGES; i++){
    if((*ptr).tid == tid){
      (*ptr).tid = 0;
      (*ptr).index = 0;
    }
    ptr += 1;
  }
}

void protectAll(){
  // Memprotect memory space - FOR THREAD PAGES ONLY
  char * memProt = mem;
  int i = 0;
  for(i = 0; i < THREAD_PAGES; i++){
    mprotect(memProt, PAGE_SIZE, PROT_NONE);  //disallow all accesses of address buffer over length pagesize
    memProt += PAGE_SIZE;
  }
}

void internalSwapper(uint in, uint out){
  if(in >= THREAD_PAGES || out >= THREAD_PAGES){
    // ERROR
    return;
  }

  // Find & allow access to memory locations
  char * inPtr = mem + in*PAGE_SIZE;
  char * outPtr = mem + out*PAGE_SIZE;
  mprotect(inPtr, PAGE_SIZE, PROT_READ | PROT_WRITE); //allow read and write to address buffer over length pagesize
  mprotect(outPtr, PAGE_SIZE, PROT_READ | PROT_WRITE); //allow read and write to address buffer over length pagesize

  // Set up temps
  char tempP[PAGE_SIZE];
  char * temPtr = tempP;
  pageInfo tempPI;

  // Copy actual data
  temPtr = memcpy(temPtr, outPtr, PAGE_SIZE);
  outPtr = memcpy(outPtr, inPtr, PAGE_SIZE);
  inPtr = memcpy(inPtr, temPtr, PAGE_SIZE);

  // Protect what was swapped out
  if(out != in)
    mprotect(inPtr, PAGE_SIZE, PROT_NONE);  //disallow all accesses of address buffer over length pagesize

  // Change Page Table
  tempPI = m_front[out];
  m_front[out] = m_front[in];
  m_front[in] = tempPI;
}

static void seghandler(int sig, siginfo_t *si, void *unused) {
  //printf("Got SIGSEGV at address: 0x%lx\n",(long) si->si_addr);

  uint tid;
  char * accessed = si->si_addr;

  // Determine WHO tried to access
  if(currCtxt == NULL){ // HASN'T MALLOCED, must be actual segfault
    // actual segfault
    return; // MAY CAUSE LOOPING
  }

  tid = (*currCtxt).data.tID;

  // WHERE was the access?
  if(accessed < mem || accessed > mem + THREAD_PAGES*PAGE_SIZE){ // Out of bounds
    // actual segfault
    return; // MAY CAUSE LOOPING
  }

  int index = 1;
  for(index = 1; index <= THREAD_PAGES; index++){
    if(accessed < mem + index*PAGE_SIZE){
      index = index-1;
      break;
    }
  }

  // Check if we need a swap
  if(m_front[index].tid == tid && m_front[index].index == index){ // If whats there is right
    mprotect(mem + index*PAGE_SIZE, PAGE_SIZE, PROT_READ | PROT_WRITE); // Un-mempotect and go
    return;

  } else { // Find the right page and swap it in
    int swapIndex = 0;
    for(swapIndex = 0; swapIndex < THREAD_PAGES; swapIndex++){
      if(m_front[swapIndex].tid == tid && m_front[swapIndex].index == index){
        break;
      }
    }

    if(swapIndex == THREAD_PAGES){
      // actual segfault
      return; // MAY CAUSE LOOPING
    }

    internalSwapper(swapIndex, index);
  }
}

/*  __createMeta()__
 *  - Fills in the metadata before a malloced chunk
 */
void * createMeta(void * start, int newSize, mb * newNextBlock){
  mb * placeData = (mb *) start;
  (*placeData).size = newSize;
  (*placeData).next = newNextBlock;

  return (void *) (placeData + 1);
}

/* MAIN FUNCTIONS */

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

  //fprintf(stderr, "ERROR: Not enough space to malloc. - File: %s, Line: %d\n", file, line);
  return NULL;
}

void * myallocate(int size, char *  file, int line, int type){
  struct itimerval res = disableTimer(); // PAUSE TIMER

  uint tid;
  mb ** currFront;

  // Has memory been created yet?
  if(mem == NULL){
    mem = (char *) memalign(PAGE_SIZE, ARRSIZE);
    if(mem == NULL){
      //ERROR
      return NULL;
    }

    // Memprotect memory space
    protectAll();

    // Set other mem pointers
    s_mem = mem + (THREAD_PAGES)*PAGE_SIZE;
    l_mem = mem + (THREAD_PAGES+SHARED_PAGES)*PAGE_SIZE;

    // Clear Memory Page table space + set table front
    m_front = (pageInfo *) ((char *)(mem + PAGE_SIZE*(TOTAL_PAGES-M_TABLE_PAGES)));

    pageInfo * temp = m_front;
    int i = 0;
    for(i = 0; i < THREAD_PAGES; i++){
      (*temp).tid = 0;
      (*temp).index = 0;
      temp = temp + 1;
    }

    // Clear File Page table space + set table front
    f_front = (pageInfo *) ((char *)(mem + PAGE_SIZE*(TOTAL_PAGES-M_TABLE_PAGES-F_TABLE_PAGES)));

    temp = f_front;
    for(i = 0; i < TOTAL_FILE_PAGES; i++){
      (*temp).tid = 0;
      (*temp).index = 0;
      temp = temp + 1;
    }

    // Set up signal handler
    struct sigaction sa;
    sa.sa_flags = SA_SIGINFO;
    sigemptyset(&sa.sa_mask);
    sa.sa_sigaction = seghandler;
    if (sigaction(SIGSEGV, &sa, NULL) == -1)
    {
        printf("Fatal error setting up signal handler\n");
        exit(EXIT_FAILURE);
    }
  }

  // Who called myallocate()?
  if(type == LIBRARYREQ){ // Request from the scheduler
    return t_myallocate(size, file, line, l_mem, LIB_PAGES*PAGE_SIZE, &libFront);

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
    currFront = &((*currCtxt).data.front);
  }

  // Find thread's total space + Find 1st free page + Find num of free pages + Find thread's first page
  int numPages = 0;
  int numFreePages = 0;
  pageInfo * firstEmpty = NULL;
  pageInfo * firstPage = NULL;

  pageInfo * temp = m_front;
  int i = 0;
  for(i = 0; i < THREAD_PAGES; i++){
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

  // Make sure enough space exists for it
  char allow = 'y';
  if(PAGE_SIZE*numFreePages < size + METASIZE )
    allow = 'n'; // TRY BUT DON'T ALLOW NEW PAGE ALLOCATION


  // If it had no Pages
  if(numPages == 0){
    (*firstEmpty).tid = tid;
    (*firstEmpty).index = 0;

    numPages = 1;

    firstPage = firstEmpty;
  }

  // Try to malloc there
  void * ret = t_myallocate(size, file, line, mem, PAGE_SIZE*numPages, currFront);

  while(ret == NULL && allow == 'y'){
    // Look for next free page
    temp = m_front;
    for(i = 0; i < THREAD_PAGES; i++){
      if((*temp).tid == 0){
        (*temp).tid = tid;
        (*temp).index = numPages;
        numPages += 1;
        break;
      }
      temp = temp + 1;
    }

    if(i == THREAD_PAGES){
      // ERROR
      return NULL;
    }

    ret = t_myallocate(size, file, line, mem, PAGE_SIZE*numPages, currFront);
  }

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

  // Leave error if memory hasnt been created yet
  if(mem == NULL){
    fprintf(stderr, "ERROR: Can't free un-malloced space - File: %s, Line: %d", file, line);
    return;
  }

  // Who called mydeallocate()?
  if(type == LIBRARYREQ){ // Request from the scheduler
    t_mydeallocate(freeThis, file, line, &libFront);
    return;

  } else { //Its a thread
    if(currCtxt == NULL){ // Threads have not been created yet
      fprintf(stderr, "ERROR: Can't free un-malloced space - File: %s, Line: %d", file, line);
      return;
    }

    tid = (*currCtxt).data.tID;
    currFront = &((*currCtxt).data.front);
  }

  // Deallocate the given pointer if possible
  t_mydeallocate(freeThis, file, line, currFront);

  setitimer(WHICH, &res, NULL); // RESUME TIMER
}
