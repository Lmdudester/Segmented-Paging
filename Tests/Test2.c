#include "../my_pthread_t.h"

#define SIZE 1000

typedef struct structThing {
	int i;
	char c;
} struc;

void * f1(void * ip) {
	int * ptrs[SIZE];
	int * iPtr = (int *) ip;
	*iPtr = 1;

	int i = 0;
	for(i = 0; i < SIZE; i++){
		ptrs[i] = malloc(sizeof(int));
		//if(ptrs[i] != NULL)
		*ptrs[i] = i;
		//else
			//printf("f1 - failed malloc...\n");

		pthread_yield();
	}

	for(i = 0; i < SIZE; i++){
		if(*ptrs[i] != i)
			*iPtr = -1;

		free(ptrs[i]);

		pthread_yield();
	}

	pthread_exit(NULL);
}

void * f2(void * ip) {
	struc * ptrs[SIZE];
	int * iPtr = (int *) ip;
	*iPtr = 1;

	int i = 0;
	for(i = 0; i < SIZE; i++){
		ptrs[i] = malloc(sizeof(struct structThing));
		(*ptrs[i]).i = i;
		(*ptrs[i]).c = 'a';

		pthread_yield();
	}

	for(i = 0; i < SIZE; i++){
		if((*ptrs[i]).i != i || (*ptrs[i]).c != 'a')
			*iPtr = -1;

		free(ptrs[i]);

		pthread_yield();
	}

	pthread_exit(NULL);
}

void main(int argc, char ** argv) {
	pthread_t * t1 = malloc(sizeof(pthread_t));
	pthread_t * t2 = malloc(sizeof(pthread_t));

	void * ret;
 	void * (*f1ptr)(void *) = f1;
	void * (*f2ptr)(void *) = f2;
  	int i1 = 0, i2 = 0;


	pthread_create(t1, NULL, f1ptr, &i1);
	pthread_create(t2, NULL, f2ptr, &i2);

	if(*t1 != 0)
		pthread_join(*t1, &ret);
	else
		printf("ERROR ON t1 FREE\n");
	if(*t2 != 0)
		pthread_join(*t2, &ret);
	else
		printf("ERROR ON t2 FREE\n");

	free(t1);
	free(t2);

	printf("Complete.\n");
	printf("i1: %d, i2: %d\n", i1, i2);
}

/*	TEST 1
void * f1(void * i) {
	int * iptr = (int *)i;
	*iptr += 1;

  int * j = malloc(sizeof(int));
  *j = 6;

  printf("In f1, j = %d.\n", *j);

	pthread_exit(NULL);
}

void main(int argc, char ** argv) {
  pthread_t * t1 = malloc(sizeof(pthread_t));

  void * ret;
  void * (*f1ptr)(void *) = f1;
  int i1 = 0;

  pthread_create(t1, NULL, f1ptr, &i1);

  if(*t1 != 0)
		pthread_join(*t1, &ret);
  else
    printf("OH DEER\n");

  printf("Complete.\n");
  printf("i1: %d\n", i1);

}
*/

// #include <unistd.h>
// #include <stdio.h>
// #include <malloc.h>
// #include <sys/mman.h>
//
// #define PAGE_SIZE sysconf( _SC_PAGE_SIZE)
//
// char * mem;
//
// void main(){
//   printf("Page Size - %d\n", PAGE_SIZE);
//   printf("uint Size - %d\n", sizeof(unsigned int));
// 	printf("Pointer Size - %d\n", sizeof(char *));
//
//
//   mem = (char *) memalign(PAGE_SIZE, 1024*1024*8);
//   if(mem == NULL)
//     printf("OH NO! - memalign\n");
//
//   // Protect the second page
//   if(mprotect(mem + PAGE_SIZE, PAGE_SIZE, PROT_NONE) == -1)
//     printf("OH NO! - mprotect\n");
//
//   char * c = mem + PAGE_SIZE - 4;
//   int * i = (void *) c;
//   *i = 4;
//
// }
