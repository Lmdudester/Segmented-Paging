#include "SegPage.c"

int randomE(int * mall, int * freed){
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

void testA(){
  //Test 1
  char * ptr[50];

  int i = 0;
  for(i = 0; i < 50; i=i+2){
    ptr[i] = malloc(80);
    ptr[i+1] = malloc(81);

    free(ptr[i]);
    free(ptr[i+1]);
  }
  if(front == -1)
    printf("Success\n");
}

void testB(){
  // Test 2
  char * ptr[50];

  int i = 0;
  for(i = 0; i < 50; i=i+2){
    ptr[i] = malloc(80);
    free(ptr[i]);
    ptr[i+1] = malloc(81);
    free(ptr[i+1]);
  }

  if(front == -1)
    printf("Success\n");
}

void testC(){
  //Test 3
  char * ptrs[1000]; //Track up to 1000 pointers at once

  int i = 0; //Malloc 1 byte 1000 times
  for(;i < 200; i++){
    ptrs[i] = malloc(100);
    if(ptrs[i] == NULL) //If malloc failed
      return;
  }

  i = 0; //Free 1000 bytes
  for(;i < 200; i++){
    free(ptrs[i]);
  }
  if(front == -1)
    printf("Success\n");
}

void testD(){
  //Test 4
  char * ptr;

  int i = 0;
  for(;i < 1000; i++){ //Malloc  1 byte and free - 1000 times
    ptr = malloc(1);
    if(ptr == NULL) //If malloc failed
      return;
    free(ptr);
    if(front == -1)
      printf("Success\n");
  }
}

void testE(){
  //Test 5
  char * ptrs[1000];  //Array of pointers to malloced stuff
  int mall = 0;       //# of times malloc was called
  int freed = 0;      //# of times free was called
  int top = -1;       //Current index of what to free next (If need be)

  int i = 0;
  for(;i < 2000; i++){
      switch(randomE(&mall, &freed)){ //Determine which to do
        case 0: //Free
          free(ptrs[top]);
          top--;
          // printf("Done Free\n");
          break;

        case 1: //Malloc
          top++;
          ptrs[top] = malloc(1);
          if(ptrs[top] == NULL) {//If malloc failed
            printf("\n\nMallocation failed\n\n");
            return;
          }
          // printf("Done Malloc\n");
          break;

        default:
          // printf("DEFAULT\n");
          return; //THIS SHOULD NEVER HAPPEN
      }
  }

  if(front == -1)
    printf("Success\n");
}

int randomSize(int * taken){
  if(*taken > ARRSIZE-17){ //Just not enough room
    return -1;
  }

  if(ARRSIZE - *taken < 68) //If theres not enough room to malloc 64
      return (rand() % (ARRSIZE - *taken - 4)) + 1; //1-(max)

  return (rand() % 64) + 1; //1-64
}

int randomF(int * mall, int * freed, int * taken){
  int size = randomSize(taken); //Get a random size to malloc 0-64 bytes

  if(*mall == 1000 || size == -1){ //If we've already malloced 1000 or there's no room
    *freed += 1;
    return 0;
  }

  if(*freed == *mall){ //If we have freed everything we have malloced
    *mall += 1;
    *taken += size + 4;
    return size;
  }

  int r = rand() % 2; //Get an random number 0 (free) or 1 (malloc)

  if(r == 1){ //If we malloc, then set r to the size of the malloc
    *(mall) += 1;
    *taken += size + 4;
    r = size;
  } else    //If we free, increment freed
    *(freed) += 1;

  return r;
}
void testF(){
char * ptrs[1000];  //Array of pointers to malloced stuff
 int mall = 0;       //# of times malloc was called
 int freed = 0;      //# of times free was called
 int taken = 0;      //# of bytes currently malloced
 int top = -1;       //Current index of what to free next (If need be)

 //Randomly Choose (2000 total - 1000 free, 1000 malloc)
 int i = 0;
 for(;i < 2000; i++){
     int size = randomF(&mall, &freed, &taken);  //Choose a size first...
                                                 //(see if theres enough room)
     switch(size){
       case 0: //Free
         taken -= *(((short *) ptrs[top]) - 1);
         printf("Freeing position %d\n",top);
         free(ptrs[top]);
         top--;
         break;

       default: //Malloc <size> bytes
         top++;
         printf("mallocing:%d at Erract %d\n",size,top);
         ptrs[top] = malloc(size);
         if(ptrs[top] == NULL) //If malloc fails
           return;
         break;
     }
   }
   if(front == -1)
     printf("Success\n");
}

void testG(){
  char *ptr[100];
  printf("Start:Test G\n");
  ptr[0] = malloc(40);
  ptr[1] = malloc(7);
  ptr[2] = malloc(52);
  free(ptr[2]);
  ptr[2] = malloc(45);
  free(ptr[2]);
  free(ptr[1]);
  ptr[1] = malloc(60);
  free(ptr[1]);
  free(ptr[0]);
  ptr[0] = malloc(57);
  free(ptr[0]);
  ptr[0] = malloc(40);
  free(ptr[0]);
  ptr[0] = malloc(27);
  ptr[1] = malloc(47);
  ptr[2] = malloc(52);
  free(ptr[2]);
  free(ptr[1]);
  ptr[1] = malloc(14);
  free(ptr[1]);
  free(ptr[0]);
  ptr[0] = malloc(38);
  ptr[1] = malloc(30);
  free(ptr[1]);
  free(ptr[0]);
  ptr[0] = malloc(21);
  free(ptr[0]);
  ptr[0] = malloc(14);
  ptr[1] = malloc(7);
  free(ptr[1]);
  free(ptr[0]);
  ptr[0] = malloc(21);
  free(ptr[0]);
  ptr[0] = malloc(14);
  ptr[1] = malloc(7);
  free(ptr[1]);
  free(ptr[0]);
  ptr[0] = malloc(3);
  ptr[1] = malloc(53);
  ptr[2] = malloc(34);
  ptr[3] = malloc(29);
  ptr[4] = malloc(49);
  ptr[5] = malloc(63);
  ptr[6] = malloc(2);
  ptr[7] = malloc(61);

  //There seemed to be a bug, thus this randomly generated ptr was formed. Fixed the bug
}
int main(int argc, char * argv[]){
  testG();

	return 0;
}
