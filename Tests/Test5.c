#include "../my_pthread_t.h"

void * f1(void * i) {
	int * sharedRet = shalloc(sizeof(int));
  *sharedRet = 5;
	pthread_exit(sharedRet);
}


void main(int argc, char ** argv) {
	pthread_t t1;
	void * (*f1ptr)(void *) = f1;

	pthread_create(&t1, NULL, f1ptr, NULL);

	void * ret;

	if(t1 != 0)
		pthread_join(t1, &ret);

	printf("Complete. - %d\n", *((int*) ret));

	free(ret);
}
