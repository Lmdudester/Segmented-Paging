#include "SegPage.h"

static char mem[ARRSIZE];
int front = -1;
// int back = -1;
int recentMal = -1;

void * createMeta(int start, int newSize, mb * newNextBlock){
  printf("start - %d | size - %d\n",start, newSize);
  if(start < 0 || start + newSize + METASIZE - 1 >= ARRSIZE){
    fprintf(stderr, "ERROR: Out of Bounds, F: %s, L: %d -\n",__FILE__, __LINE__);
    return NULL;
  }

	mb* tmp = (mb*) &mem[start];
  (*tmp).nextBlock = newNextBlock;
	(*tmp).size = newSize;
	(*tmp).loc = start;

	// int * ptr = (int *) mem+start; //Gives a pointer after the metadata
  return (void *) tmp;
}

void * myallocate(int size, char *  file, int line, int type){
  if(size < 1 || size > ARRSIZE) {
		fprintf(stderr, "ERROR: Can't malloc < 0 or greater then %d byte - File: %s, Line: %d", ARRSIZE, file, line);
	}
	if(front == -1) { //No blocks allotted
    front = 0;
    recentMal = 0;
    return createMeta(0,size,NULL);
  }

	/*Check the below code after getting multiple allocation down. You'll need fuck, umm you'll need to have mydeallocate
	ready to deallocate the first big struct after the small struct*/
	if(size + METASIZE - 1 < front){ //Check before first allocated block
    mb* frnt = (mb*) &mem[front]; // Strucures front
		front = 0;
		return createMeta(0,size,frnt); //Start at 0, next is old front
	}

  mb* frnt = (mb*) &mem[front]; // Strucures front
  if((*frnt).nextBlock == NULL){ //If there's only one block of memory in the list
    printf("0\n");
    if((*frnt).size + 2*METASIZE + size - 1 < ARRSIZE){ //If the size of both fits in memory
      recentMal = front+(*frnt).size+METASIZE; //Go RIGHT AFTER the first (front) block
      (*frnt).nextBlock = (void *) &mem[front+(*frnt).size+METASIZE]; //Front should point the block to the next block
      return createMeta(front+(*frnt).size+METASIZE, size, NULL); //Create that
    }

    //Otherwise - Send an error
    fprintf(stderr, "ERROR: Not enough memory - File: %s, Line: %d", file, line);
    return NULL;
  }

  //Set up tracking variables
  mb* prev = (mb*) &mem[front];
  mb* curr = (mb*) (*prev).nextBlock;
  int prevLoc = (*prev).loc;
  int currLoc = (*curr).loc;

  while((*curr).nextBlock != NULL){ //We're looking between two Chunks -- EXCEPT LAST TWO
    // printf("1\n");
    printf("prevLoc:%d\t",prevLoc);
    printf("currLoc:%d\t",currLoc);

    printf("Less than (*curr).size%d\n",(*curr).size);
    if(prevLoc + (*prev).size + 2*METASIZE + size - 1 < currLoc){ //If it fits between them
      (*prev).nextBlock = (void *)&mem[prevLoc + (*prev).size + METASIZE];
      recentMal = prevLoc + (*prev).size + METASIZE;
      return createMeta(prevLoc + (*prev).size + METASIZE, size, curr);
    }

    //Increment tracking variable
    prev = curr;
    prevLoc = (*prev).loc;
    curr = (mb*) (*prev).nextBlock;
    currLoc = (*curr).loc;
  }
  //After checking through, this will put it at the between of very end of two nodes
  if(prevLoc + (*prev).size + 2*METASIZE + size - 1 < currLoc){
    // printf("2\n");
    (*prev).nextBlock = (void *)&mem[prevLoc + (*prev).size + METASIZE];
    recentMal = prevLoc + (*prev).size + METASIZE;
    return createMeta(prevLoc + (*prev).size + METASIZE, size, curr);
  }

  //After checking thorugh, this will put it at the very end
  if(currLoc + (*curr).size + 2*METASIZE + size - 1 < ARRSIZE){
    // printf("3\n",size);
    (*curr).nextBlock = (void *)&mem[currLoc + (*curr).size + METASIZE];
    recentMal = currLoc + (*curr).size + METASIZE;
    return createMeta(recentMal, size, NULL);
  }

  fprintf(stderr, "ERROR: Not enough memory - File: %s, Line: %d", file, line);
  return NULL;
}

void * mydeallocate(void * freeThis, char * file, int line, int type){
	if(front == -1){
    fprintf(stderr, "ERROR: No malloced chunks to free - File: %s, Line: %d\n", file, line);
    return;
  }

  mb *ptr = (mb*) &mem[front];
  if(freeThis == (void *)ptr){
    // printf("Front Check\n");
    mb* nextFront = (mb*) (*ptr).nextBlock;

    if((*ptr).nextBlock != NULL){
      front = (*nextFront).loc;
    } else {
      front = -1;
    }

    recentMal = -1;
    return;
	}

  //Freeing memory that's in the middle
  mb* prev = (mb*) &mem[front];
  mb* curr = (mb*) (*prev).nextBlock;

  //Checking the middle
  while((*curr).nextBlock != NULL){
    // printf("Middle Check\n");
    if(freeThis == (void *) curr){
      (*prev).nextBlock = (*curr).nextBlock; //Point prv pointer to the next of the current chunk
      recentMal = (*prev).loc;
      return;
    }

    //Increment tacking variables
    prev = curr;
    curr = (mb*) (*prev).nextBlock;
  }

  //Freeming memory that's at the end
  if(freeThis == (void *) curr){
    // printf("End Check\n");
    (*prev).nextBlock = NULL; //Point prv pointer to the next of the current chunk
    recentMal = (*prev).loc;
    return;
  }

  fprintf(stderr, "ERROR: Pointer was not malloced - File: %s, Line: %d\n", file, line);
  return;
}
