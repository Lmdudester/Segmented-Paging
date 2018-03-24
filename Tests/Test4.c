#include "../my_pthread_t.h"

void * f1(void * ip) {
	char * ptr = malloc(4177000);
  char * temp = ptr;

	pthread_yield();

  (*temp) = 'a';
  temp += 1000000;
  (*temp) = 'b';
  temp += 1000000;
  (*temp) = 'c';
  temp += 1000000;
  (*temp) = 'd';
  temp += 1000000;
  (*temp) = 'e';

  temp = ptr;

  pthread_yield();

  if((*temp) != 'a')
    exit(-1);
  temp += 1000000;

  if((*temp) != 'b')
    exit(-1);
  temp += 1000000;

  if((*temp) != 'c')
    exit(-1);
  temp += 1000000;

  if((*temp) != 'd')
    exit(-1);
  temp += 1000000;

  if((*temp) != 'e')
    exit(-1);

	pthread_yield();

  free(ptr);

	pthread_exit(NULL);
}

void main(int argc, char ** argv) {
  pthread_t tid1, tid2;
  int res1, res2;

  void * ret;
 	void * (*f1ptr)(void *) = f1;


  // Create threads
  pthread_create(&tid1, NULL, f1ptr, &res1);
  pthread_create(&tid2, NULL, f1ptr, &res2);

  // Join on all created threads
  pthread_join(tid1, &ret);
  pthread_join(tid2, &ret);

  printf("Success.\n");
}
