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
    if((*frnt).size + 2*METASIZE + size - 1 < ARRSIZE){ //If the size of both fits in memory
      recentMal = front+(*frnt).size+METASIZE; //Go RIGHT AFTER the first (front) block
      (*frnt).nextBlock = (void *) &mem[recentMal]; //Front should point the block to the next block

      return createMeta(recentMal, size, NULL); //Create that
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
    if(prevLoc + (*prev).size + 2*METASIZE + size - 1 < (*curr).size){ //If it fits between them
      (*prev).nextBlock = (void *)&mem[prevLoc + (*prev).size + METASIZE];
      recentMal = prevLoc + (*prev).size + METASIZE;
      return createMeta(prevLoc + (*prev).size + METASIZE, size, curr);
    }

    //Increment tracking variables
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


int randomC(int * mall, int * freed){
  if(*mall == 1000){ //If we've malloced a total of 1000 already
    *freed += 1;
    return 0;
  }
  if(*freed == *mall){ //If we've freed everything we've malloced
    *mall += 1;
    return 1;
  }

  int r = rand() % 2; //Get a random number, 0 or 1

  if(r == 1) //If one, we're going to malloc so increase the counter
    *(mall) += 1;
  else      //If zero, we're going to free so increase the counter
    *(freed) += 1;

  return r; //Return the random number
}

int main(int argc, char * argv[]){
  // //Test 1
	// char * ptr[50];
  //
  // int i = 0;
  // for(i = 0; i < 50; i=i+2){
  //   ptr[i] = malloc(80);
  //   ptr[i+1] = malloc(81);
  //
  //   free(ptr[i]);
  //   free(ptr[i+1]);
  // }
  // if(front == -1)
  //   printf("Success\n");

  // Test 2
  // char * ptr[50];
  //
  // int i = 0;
  // for(i = 0; i < 50; i=i+2){
  //   ptr[i] = malloc(80);
  //   free(ptr[i]);
  //   ptr[i+1] = malloc(81);
  //   free(ptr[i+1]);
  // }
  //
  // if(front == -1)
  //   printf("Success\n");


  // //Test 3
  // char * ptrs[1000]; //Track up to 1000 pointers at once
  //
  // int i = 0; //Malloc 1 byte 1000 times
  // for(;i < 200; i++){
  //   ptrs[i] = malloc(100);
  //   if(ptrs[i] == NULL) //If malloc failed
  //     return 1;
  // }
  //
  // i = 0; //Free 1000 bytes
  // for(;i < 200; i++){
  //   free(ptrs[i]);
  // }
  // if(front == -1)
  //   printf("Success\n");

  // //Test 4
  // char * ptr;
  //
  // int i = 0;
  // for(;i < 1000; i++){ //Malloc  1 byte and free - 1000 times
  //   ptr = malloc(1);
  //   if(ptr == NULL) //If malloc failed
  //     return 1;
  //   free(ptr);
  //   if(front == -1)
  //     printf("Success\n");
  // }

  // //Test 5
  // char * ptrs[1000];  //Array of pointers to malloced stuff
  // int mall = 0;       //# of times malloc was called
  // int freed = 0;      //# of times free was called
  // int top = -1;       //Current index of what to free next (If need be)
  //
  // //Randomly Choose (2000 total - 1000 free, 1000 malloc)
  // int i = 0;
  // for(;i < 2000; i++){
  //     switch(randomC(&mall, &freed)){ //Determine which to do
  //       case 0: //Free
  //         free(ptrs[top]);
  //         top--;
  //         break;
  //
  //       case 1: //Malloc
  //         top++;
  //         ptrs[top] = malloc(1);
  //         if(ptrs[top] == NULL) //If malloc failed
  //           printf("\n\nMallocation failed\n\n");
  //            return 0;
  //         break;
  //
  //       default:
  //         return 1; //THIS SHOULD NEVER HAPPEN
  //     }
  // }
  //
  // if(front == -1)
  //   printf("Success\n");

	return 0;
}
