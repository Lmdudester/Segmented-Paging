#include "../my_pthread_t.h"

// NOTE: This test takes a long time, it creates 62 threads and mallocs 1000 ints for each
//        constantly thrashing the whole time

#define SIZE 1000

void * f1(void * ip) {
	int * ptrs[SIZE];
	int * iPtr = (int *) ip;
	*iPtr = 1;

	int i = 0;
	for(i = 0; i < SIZE; i++){
		ptrs[i] = malloc(sizeof(int)*20);
		*ptrs[i] = i;


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

void main(int argc, char ** argv) {
  pthread_t tids[100];
  int results[100];

  void * ret;
 	void * (*f1ptr)(void *) = f1;

  // Set all results to 0;
  int i = 0;
  for(i=0; i < 100; i++){
    results[i] = 0;
  }

  // Create up to 100 threads - STOP as soon as a failure happens
  i = 0;
  for(i = 0; i < 100; i++){
    if(pthread_create(&tids[i], NULL, f1ptr, &results[i]) == -1)
      break;
  }

  // Join on all created threads
  int n = 0;
  for(n = 0; n < i; n++){
    pthread_join(tids[n], &ret);
  }

  // Check for success
  n = 0;
  for(n = 0; n < i; n++){
    if(results[n] != 1){
      printf("Failure.\n");
      return;
    }
  }

  printf("Success. Created %d threads.\n", (i-1));
}
