// File:	SegPage.c
// Authors: Liam Davies, Kevin Lee, Brian Ellsworth
// username of iLab: lmd312, kjl156, bje40
// iLab Server: factory.cs.rutgers.edu

#include "SegPage_t.h"

/* Pointers to the start of each section of memory */
char * mem = NULL;
char * s_mem = NULL;
char * l_mem = NULL;

pageInfo * m_front = NULL;
pageInfo * f_front = NULL;

/* Malloc "front" pointers for library and shalloc requests */
mb * libFront = NULL;
mb * sharedFront = NULL;

/* File descriptor for the swapfile */
int swapfd;


/* TESTING-ONLY FUNCTIONS*/

/* __printPT()__
 *	Prints out the first n entries of the memory page table.
 *	Args:
 *		- int howMany - the number of entries to print
 *	Returns:
 *		- N/A
 */
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


/* HELPER METHODS */

/* __removePages()__
 *	Clears all page table metadata for a given tid
 *	Args:
 *		- uint tid - the tid to delete all pages of
 *	Returns:
 *		- N/A
 */
void removePages(uint tid){
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

/* __protectAll()__
 *	Calls mprotect on all thread pages
 *	Args:
 *		- N/A
 *	Returns:
 *		- N/A
 */
void protectAll(){
  // Memprotect memory space - FOR THREAD PAGES ONLY
  char * memProt = mem;
  int i = 0;
  for(i = 0; i < THREAD_PAGES; i++){
    mprotect(memProt, PAGE_SIZE, PROT_NONE);  //disallow all accesses of address buffer over length pagesize
    memProt += PAGE_SIZE;
  }
}

/* __internalSwapper()__
 *	Swaps two pages and un-mprotects in. (MEM->MEM only)
 *	Args:
 *		- uint in - the index of the page being swapped in
 *    - uint out - the index of the page being swapped out
 *	Returns:
 *		- N/A
 */
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

/* __memToFile()__
 *	Swaps two pages and un-mprotects in. (MEM->FILE only)
 *	Args:
 *		- uint in - the index of the page being swapped in
 *    - uint out - the index of the page being swapped out
 *	Returns:
 *		- N/A
 */
void memToFile(uint in, uint out){
  if(in >= THREAD_PAGES || out >= TOTAL_FILE_PAGES){
    // ERROR
    return;
  }

  // Find & allow access to memory locations
  char * inPtr = mem + in*PAGE_SIZE;
  mprotect(inPtr, PAGE_SIZE, PROT_READ | PROT_WRITE); //allow read and write to address buffer over length pagesize

  // Set up temps
  char tempP[PAGE_SIZE];
  char * temPtr = tempP;
  pageInfo tempPI;

  // Read from swapfd
  int total = out*PAGE_SIZE;
  while(total > 0){
    total = total - lseek(swapfd, total, SEEK_CUR);
  }

  total = PAGE_SIZE;
  while(total > 0){
    total = total - read(swapfd, temPtr, total);
  }

  lseek(swapfd, 0, SEEK_SET); // Reset swapfd to front of swapfile

  // Write in data to swap file
  total = out*PAGE_SIZE;
  while(total > 0){
    total = total - lseek(swapfd, total, SEEK_CUR);
  }

  total = PAGE_SIZE;
  while(total > 0){
    total = total - write(swapfd, inPtr, total);
  }

  lseek(swapfd, 0, SEEK_SET); // Reset swapfd to front of swapfile

  // Copy from buffer
  inPtr = memcpy(inPtr, temPtr, PAGE_SIZE);


  // Alter Page tables
  tempPI = f_front[out];
  f_front[out] = m_front[in];
  m_front[in] = tempPI;
}

/* __createMeta()__
 *	Given the info needed, populates the given space with metadata and returns
 *    a pointer to the actual chunk of memory directly after it
 *	Args:
 *		- void * start - pointer to where the metadata is going
 *    - int newSize - size of the allocated chunk
 *    - mb * newNextBlock - pointer to the next metadata block
 *	Returns:
 *		- void * - a pointer to the beginning of the associated memory
 */
void * createMeta(void * start, int newSize, mb * newNextBlock){
  mb * placeData = (mb *) start;
  (*placeData).size = newSize;
  (*placeData).next = newNextBlock;

  return (void *) (placeData + 1);
}


/* SEGMENTATION FAULT HANDLER */

/* __seghandler()__
 *	Checks if its a real segfault or a page swap and swaps pages if needed
 *	Args:
 *		- int sig - the signal number
 *    - siginfo_t *si - used to access the address of the segfault
 *    - void *unused - N/A
 *	Returns:
 *		- N/A
 */
static void seghandler(int sig, siginfo_t *si, void *unused) {
  struct itimerval res = disableTimer(); // PAUSE TIMER
  //printf("Got SIGSEGV at address: 0x%lx\n",(long) si->si_addr);

  uint tid;
  char * accessed = si->si_addr;

  // Determine WHO tried to access
  if(currCtxt == NULL){ // HASN'T MALLOCED, must be actual segfault
    // actual segfault
    setitimer(WHICH, &res, NULL); // RESUME TIMER
    return; // MAY CAUSE LOOPING
  }

  tid = (*currCtxt).data.tID;

  // WHERE was the access?
  if(accessed < mem || accessed > mem + THREAD_PAGES*PAGE_SIZE){ // Out of bounds
    // actual segfault
    setitimer(WHICH, &res, NULL); // RESUME TIMER
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
    setitimer(WHICH, &res, NULL); // RESUME TIMER
    return;

  } else { // Find the right page and swap it in
    // In memory
    int swapIndex = 0;
    for(swapIndex = 0; swapIndex < THREAD_PAGES; swapIndex++){
      if(m_front[swapIndex].tid == tid && m_front[swapIndex].index == index){
        internalSwapper(swapIndex, index);
        setitimer(WHICH, &res, NULL); // RESUME TIMER
        return;
      }
    }

    // In swapfile
    swapIndex = 0;
    for(swapIndex = 0; swapIndex < TOTAL_FILE_PAGES; swapIndex++){
      if(f_front[swapIndex].tid == tid && f_front[swapIndex].index == index){
        memToFile(index, swapIndex);
        setitimer(WHICH, &res, NULL); // RESUME TIMER
        return;
      }
    }

    if(swapIndex == THREAD_PAGES){
      // actual segfault
      setitimer(WHICH, &res, NULL); // RESUME TIMER
      return; // MAY CAUSE LOOPING
    }
  }
}


/* MAIN FUNCTIONS */

/* __t_myallocate()__
 *	Attempts to allocate space in a given region, generating appropriate metadata
 *	Args:
 *		- size_t size - the size of the memory space that is being requested
 *    - char *  file/int line - directives for error displaying
 *    - char * memStart - pointer to the start of the space in which malloc is being called
 *    - size_t memSize - the size of the memory space in which we have to malloc
 *    - mb ** frontPtr - pointer to the pointer to the front of the metadata linked list
 *	Returns:
 *		- void * - a pointer to the beginning of the associated memory
 *    - NULL - if it cannot be done
 */
void * t_myallocate(size_t size, char *  file, int line, char * memStart, size_t memSize, mb ** frontPtr){
  if(size < 1 || size + METASIZE > memSize) {
		//fprintf(stderr, "ERROR: Can't malloc < 0 or greater then %d byte - File: %s, Line: %d", (memSize - METASIZE), file, line);
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

/* __myallocate()__
 *	Deals with all of the paging aspects so t_myallocate can assume contiguous space
 *	Args:
 *		- size_t size - the size of the memory space that is being requested
 *    - char *  file/int line - directives for error displaying
 *    - int type - THREADREQ/LIBRARYREQ
 *	Returns:
 *		- void * - a pointer to the beginning of the associated memory
 *    - NULL - if it cannot be done
 */
void * myallocate(size_t size, char *  file, int line, int type){
  struct itimerval res = disableTimer(); // PAUSE TIMER

  uint tid;
  mb ** currFront;
  int i;
  void * ret;

  // Has memory been created yet?
  if(mem == NULL){
    mem = (char *) memalign(PAGE_SIZE, ARRSIZE);
    if(mem == NULL){
      //ERROR
      setitimer(WHICH, &res, NULL); // RESUME TIMER
      return NULL;
    }

    // Memprotect memory space
    protectAll();

    // Create swapfile
    swapfd = open("mem.dat", O_CREAT | O_RDWR | O_TRUNC);
    if(swapfd == -1){
      //ERROR
      setitimer(WHICH, &res, NULL); // RESUME TIMER
      return NULL;
    }

    // Reset swapfd to front  pf swapfile
    lseek(swapfd, 0, SEEK_SET);

    // Size the swapfile
    i = 0;
    for(i = 0; i < TOTAL_FILE_PAGES; i++){
      int total = PAGE_SIZE;
      while(total > 0){
        total = total - write(swapfd, mem + (THREAD_PAGES)*PAGE_SIZE, PAGE_SIZE);
      }
    }

    // Reset swapfd to front  pf swapfile
    lseek(swapfd, 0, SEEK_SET);


    // Set other mem pointers
    s_mem = mem + (THREAD_PAGES)*PAGE_SIZE;
    l_mem = mem + (THREAD_PAGES+SHARED_PAGES)*PAGE_SIZE;

    // Clear Memory Page table space + set table front
    m_front = (pageInfo *) ((char *)(mem + PAGE_SIZE*(TOTAL_PAGES-M_TABLE_PAGES)));

    pageInfo * temp = m_front;
    i = 0;
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
    ret = t_myallocate(size, file, line, l_mem, LIB_PAGES*PAGE_SIZE, &libFront);
    setitimer(WHICH, &res, NULL); // RESUME TIMER
    return ret;

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
        setitimer(WHICH, &res, NULL); // RESUME TIMER
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

  // Find thread's total space + Find 1st free page + Find num of free pages
  int m_nPages = 0; // Checking memory
  int m_nFreePages = 0;
  pageInfo * m_firstEmpty = NULL;

  pageInfo * temp = m_front;
  i = 0;
  for(i = 0; i < THREAD_PAGES; i++){
    if((*temp).tid == tid) {
      m_nPages += 1;
    }

    if((*temp).tid == 0){
      if(m_firstEmpty == NULL)
        m_firstEmpty = temp;
      m_nFreePages += 1;
    }
    temp = temp + 1;
  }

  int f_nPages = 0; // Checking swapfile
  int f_nFreePages = 0;
  pageInfo * f_firstEmpty = NULL;

  temp = f_front;
  i = 0;
  for(i = 0; i < TOTAL_FILE_PAGES; i++){
    if((*temp).tid == tid) {
      f_nPages += 1;
    }

    if((*temp).tid == 0){
      if(f_firstEmpty == NULL)
        f_firstEmpty = temp;
      f_nFreePages += 1;
    }
    temp = temp + 1;
  }

  // Make sure enough space exists for it
  char allow = 'y';
  if(PAGE_SIZE*(THREAD_PAGES-f_nPages-m_nPages) < size + METASIZE ||
      PAGE_SIZE*(f_nFreePages+m_nFreePages) < size + METASIZE)
    allow = 'n'; // TRY BUT DON'T ALLOW NEW PAGE ALLOCATION

  // If it had no Pages
  if(m_nPages + f_nPages == 0){
    if(m_firstEmpty != NULL){ // Pages exist in memory
      (*m_firstEmpty).tid = tid;
      (*m_firstEmpty).index = 0;

      m_nPages = 1;
      m_nFreePages -= 1;

    } else if (f_firstEmpty != NULL) { // Pages exist in swapfile
      (*f_firstEmpty).tid = tid;
      (*f_firstEmpty).index = 0;

      f_nPages = 1;
      f_nFreePages -= 1;

    } else { // If there are no free pages
      // ERROR
      setitimer(WHICH, &res, NULL); // RESUME TIMER
      return NULL;
    }
  }

  // Try to malloc there
  ret = t_myallocate(size, file, line, mem, PAGE_SIZE*(m_nPages+f_nPages), currFront);

  while(ret == NULL && allow == 'y'){
    if(m_nFreePages > 0){
      // Look for next free page - in mem
      temp = m_front;
      for(i = 0; i < THREAD_PAGES; i++){
        if((*temp).tid == 0){
          (*temp).tid = tid;
          (*temp).index = m_nPages + f_nPages;
          m_nPages += 1;
          m_nFreePages -= 1;
          break;
        }
        temp = temp + 1;
      }
    } else if(f_nFreePages > 0) {
      // Look for next free page - in swapfile
      temp = f_front;
      for(i = 0; i < TOTAL_FILE_PAGES; i++){
        if((*temp).tid == 0){
          (*temp).tid = tid;
          (*temp).index = m_nPages + f_nPages;
          f_nPages += 1;
          f_nFreePages -= 1;
          break;
        }
        temp = temp + 1;
      }
    } else {
      // ERROR
      setitimer(WHICH, &res, NULL); // RESUME TIMER
      return NULL;
    }

    ret = t_myallocate(size, file, line, mem, PAGE_SIZE*(m_nPages+f_nPages), currFront);
  }

  setitimer(WHICH, &res, NULL); // RESUME TIMER
  return ret;
}

/* __myshalloc()__
 *	Attempts to obtain space in a shared region so that multiple threads may access it
 *	Args:
 *		- size_t size - the size of the memory space that is being requested
 *    - char *  file/int line - directives for error displaying
 *	Returns:
 *		- void * - a pointer to the beginning of the associated memory
 *    - NULL - if it cannot be done
 */
void * myshalloc(size_t size, char *  file, int line){
  struct itimerval res = disableTimer(); // PAUSE TIMER

  if(mem == NULL){
    // ERROR
    setitimer(WHICH, &res, NULL); // RESUME TIMER
    return NULL;
  }

  void * ret = t_myallocate(size, file, line, s_mem, SHARED_PAGES*PAGE_SIZE, &sharedFront);

  setitimer(WHICH, &res, NULL); // RESUME TIMER
  return ret;
}

/* __t_mydeallocate()__
 *	Attempts to free the metadata of a given mallcoed pointer
 *	Args:
 *		- void * freeThis - the pointer to the memory to free
 *    - char *  file/int line - directives for error displaying
 *    - mb ** frontPtr - pointer to the pointer to the front of the metadata linked list
 *    - char shalloc - set to 's' when checking shalloc memory so no errors are printed
 *	Returns:
 *		- int - 0 to indicate success, -1 for failure
 */
int t_mydeallocate(void * freeThis, char * file, int line, mb ** frontPtr, char shalloc){
	if((*frontPtr) == NULL){
    if(shalloc != 's')
      fprintf(stderr, "ERROR: No malloced chunks to free - File: %s, Line: %d\n", file, line);
    return -1;

  } else if(freeThis == NULL){
    if(shalloc != 's')
      fprintf(stderr, "ERROR: Cannot free a NULL pointer - File: %s, Line: %d\n", file, line);
    return -1;
  }

  // First metaBlock
  if(freeThis == (void *)((*frontPtr) + 1)){
    (*frontPtr) = (*(*frontPtr)).next;
    return 0;
  }

  // Middle + End metaBlocks
  mb * ptr = (*frontPtr);
  while((*ptr).next != NULL){
    if(freeThis == (void *)((*ptr).next + 1)){
      (*ptr).next = (*((*ptr).next)).next;
      return 0;
    }

    ptr = (*ptr).next;
  }

  if(shalloc != 's')
    fprintf(stderr, "ERROR: Cannot free an unmalloced pointer - File: %s, Line: %d\n", file, line);
  return -1;
}

/* __mydeallocate()__
 *	Deals with all of the paging aspects so t_mydeallocate can assume contiguous space
 *	Args:
 *		- void * freeThis - the pointer to the memory to free
 *    - char *  file/int line - directives for error displaying
 *    - int type - THREADREQ/LIBRARYREQ
 *	Returns:
 *		- N/A
 */
void mydeallocate(void * freeThis, char * file, int line, int type){
  struct itimerval res = disableTimer(); // PAUSE TIMER

  uint tid;
  mb ** currFront;

  // Leave error if memory hasnt been created yet
  if(mem == NULL){
    fprintf(stderr, "ERROR: Can't free un-malloced space - File: %s, Line: %d", file, line);
    setitimer(WHICH, &res, NULL); // RESUME TIMER
    return;
  }

  // Check if it was shalloced
  if(t_mydeallocate(freeThis, file, line, &sharedFront, 's') == 0){
    setitimer(WHICH, &res, NULL); // RESUME TIMER
    return;
  }

  // Who called mydeallocate()?
  if(type == LIBRARYREQ){ // Request from the scheduler
    t_mydeallocate(freeThis, file, line, &libFront, 'r');
    setitimer(WHICH, &res, NULL); // RESUME TIMER
    return;

  } else { //Its a thread
    if(currCtxt == NULL){ // Threads have not been created yet
      fprintf(stderr, "ERROR: Can't free un-malloced space - File: %s, Line: %d", file, line);
      setitimer(WHICH, &res, NULL); // RESUME TIMER
      return;
    }

    tid = (*currCtxt).data.tID;
    currFront = &((*currCtxt).data.front);
  }

  // Deallocate the given pointer if possible
  t_mydeallocate(freeThis, file, line, currFront, 'r');

  setitimer(WHICH, &res, NULL); // RESUME TIMER
}
