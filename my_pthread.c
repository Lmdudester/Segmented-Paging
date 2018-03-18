// File:	my_pthread.c
// Author:	Yujie REN
// Date:	09/23/2017

// name: Liam Davies, Kevin Lee, Brian Ellsworth
// username of iLab: lmd312, kjl156, bje40
// iLab Server: factory

#include "my_pthread_t.h"

// Queues
tcbNode * q1 = NULL;
tcbNode * q2 = NULL;
tcbNode * q3 = NULL;

// Linked Lists
tcbNode * completed = NULL;

// The currently running context
tcbNode * currCtxt = NULL;

// For threadID generation
my_pthread_t idCount = 1;

// For Maintenance Cycle decisions
int numMaintain = 0;


/* HELPER FUNCTIONS */

struct itimerval disableTimer(){
	struct itimerval timer, ret;
	timer.it_value.tv_sec = 0; // 0 seconds
  timer.it_value.tv_usec = 0; // 0 milliseconds
	setitimer(WHICH, &timer, &ret); // CANCEL TIMER
	return ret;
}

/* __enqueue()__
 *	Adds the newly created node to the parameter queue.
 *	Args:
 *		- tcbNode ** queue - a pointer to q1, q2 or q3 (use &q1, &q2, &q3, &completed)
 *		- tcbNode * newNode - the node to be inserted
 *	Returns:
 *		- N/A
 * NOTE: Assumes all newNode(s) have had their next ptr set to NULL
 */
void enqueue(tcbNode ** queue, tcbNode * newNode){
	if(*queue == NULL) { // Its empty
		(*queue) = newNode;
		return;
	}

	// 1-More Items
	tcbNode * ptr = (*queue);
	while((*ptr).next != NULL) {
		ptr = (*ptr).next;
	}
	(*ptr).next = newNode;
}

 /* __dequeue()__
  *	Returns the top tcbNode from the parameter queue.
  *	Args:
  *		- tcbNode ** queue - a pointer to q1, q2 or q3 (use &q1, &q2, &q3)
  *	Returns:
  *		- tcbNode * - the top tcbNode
	*		- NULL - if no nodes exist
	* NOTE: To meet NULL requirement of enqueue, returned node has a next of NULL
  */
tcbNode * dequeue(tcbNode ** queue){
	if(*queue == NULL) // Its empty
		return NULL;

	tcbNode * ret = *queue;
	*queue = (**queue).next;
	(*ret).next = NULL;
	return ret;
}

tcbNode * getExitThread(my_pthread_t id){
		tcbNode * ptr = completed;
		tcbNode * prv = NULL;

		while(ptr != NULL) { // Look at each element
			if((*ptr).data.tID == id)
				break;
			prv = ptr;
			ptr = (*ptr).next;
		}

		if(ptr == NULL) // It doesn't exist
			return NULL;

		if(prv == NULL){ // Its the first element
			completed = (*ptr).next;
			return ptr;
		}

		(*prv).next = (*ptr).next; // Otherwise
		return ptr;
}

/* SCHEDULER FUNCTIONS */

/* __isThreadDone()__
 *	Determines if a thread is done based on it's tID
 *	Args:
 *		- my_pthread_t id - the tID of the thead in question
 *	Returns:
 *		- 0 - thread is not in the "completed" linked list
 *		- 1 - thread is in the "completed" linked list
 */
int isThreadDone(my_pthread_t id){
	tcbNode * ptr = completed;
	while(ptr != NULL){
		if((*ptr).data.tID == id)
			return 1;
		ptr = (*ptr).next;
	}
	return 0;
}

/* __checkQueue()__
 *	Returns a pointer to the next runnable tcbNode in the given queue.
 *	Args:
 *		- tcbNode ** queue - a pointer to q1, q2 or q3 (use &q1, &q2, &q3)
 *	Returns:
 *		- tcbNode * - the next runnable tcbNode in the given queue
 *		- NULL - if no runnable nodes exist
 */
tcbNode * checkQueue(tcbNode ** queue){
	tcbNode * nextProc = NULL;
	tcbNode * front = *queue; // Find the current front so we can see if we cycle
	int runs = 0;

	while(nextProc == NULL){
		tcbNode * testProc = dequeue(queue);

		if(front == testProc && runs != 0) { // Cycled through the queue
			(*testProc).next = *queue;
			*queue = testProc;
			break;
		}

		if(testProc == NULL) // Nothing in queue
			break;

		switch((*testProc).data.stat){
			case P_RUN: // Set it up to go next
				nextProc = testProc;
				break;

			case P_WAIT_M:
				if((*((*testProc).data.w_mutex)).lock == 0) { // If it's mutex is unclocked
					nextProc = testProc;
					(*testProc).data.stat = P_RUN;
				} else
					enqueue(queue,testProc);
				break;

			case P_WAIT_T:
				if(isThreadDone((*testProc).data.w_tID) == 1) { // If the thread is complete
					nextProc = testProc;
					(*testProc).data.stat = P_RUN;
				} else
					enqueue(queue,testProc);
				break;

			default:
				break;
		}
		runs++;
	}

	return nextProc;
}

/* __scheduler()__
 *	Decides what to run next
 *	Args:
 *		- signum - a int repreenting the signal that was set off
 *	Returns:
 *		- N/A
 */
void scheduler(int signum){
	/* Set up sigmask */
	sigset_t toBlock;
	sigemptyset(&toBlock);
	sigaddset(&toBlock, T_SIG);
	sigprocmask(SIG_BLOCK, &toBlock, NULL);

	if((*currCtxt).data.stat == P_EXIT) // TO PREVENT RUNNING TO COMPLETION ERROR
			enqueue(&completed, currCtxt);

	/* Choose next ctxt */
	tcbNode * nextProc = checkQueue(&q1); // Queue 1
	if(nextProc == NULL)
		nextProc = checkQueue(&q2); // Queue 2
	if(nextProc == NULL)
		nextProc = checkQueue(&q3); // Queue 3

	if(nextProc == NULL) { // Then currCtxt is the only possible context, so let it continue
		numMaintain++;			 // NOTE: No timer set since nothing can run until a yield anyway
		//printf("C: %d -> NULL (q1 - %d, q2 - %d, q3 - %d, exit - %d)\n", (*currCtxt).data.tID, q1, q2, q3, completed);
		return;
	}

	//printf("C: %d -> %d (q1 - %d, q2 - %d, q3 - %d, exit - %d) \n", (*currCtxt).data.tID, (*nextProc).data.tID,  q1, q2, q3, completed);

	/* Decide which queue to send current cTxt to */
	switch((*currCtxt).data.stat){
		case P_RUN:
			if((*currCtxt).data.qNum == 1) {  // Was in q1
				(*currCtxt).data.qNum = 2;
				enqueue(&q2, currCtxt);
			} else	{ // Was in q2 or q3
				(*currCtxt).data.qNum = 3;
				enqueue(&q3, currCtxt);
			}
			break;

		case P_YIELD:
			(*currCtxt).data.stat = P_RUN;
		case P_WAIT_M:
		case P_WAIT_T:
			if((*currCtxt).data.qNum == 1) // Was in q1
				enqueue(&q1, currCtxt);
			else if((*currCtxt).data.qNum == 2) // Was in q2
				enqueue(&q2, currCtxt);
			else // Was in q3
				enqueue(&q3, currCtxt);
			break;
	}

	/* Maintenance */
	if(numMaintain < MAINTAIN){
		numMaintain++;
	} else {
		numMaintain++;
		if(q3 != NULL) { // If q3 isn't empty
			tcbNode * mProc = dequeue(&q3);
			enqueue(&q1, mProc);
		} else if(q2 != NULL) { // If q2 isnt empty
			tcbNode * mProc = dequeue(&q2);
			enqueue(&q1, mProc);
		} // Otherwise, do nothing
		numMaintain = 0;
	}

	/* Prep to Swap contexts */
	tcbNode * curr_temp = currCtxt; // Make a temp for the old currCtxt (so we can still swap)
	currCtxt = nextProc;	// Change the currCtxt pointer

	/* Reset Timer */
	struct itimerval timer;
	timer.it_interval.tv_sec = 0;	// 0 seconds
  timer.it_interval.tv_usec = 0; // 0 milliseconds - NO REPEAT
  timer.it_value.tv_sec = 0; // 0 seconds
	// (BASED ON Q#) milliseconds
	if((*currCtxt).data.qNum == 1) // q1
  	timer.it_value.tv_usec = Q1_MSECS;
	if((*currCtxt).data.qNum == 2) // q2
	 	timer.it_value.tv_usec = Q2_MSECS;
	else // q3
		timer.it_value.tv_usec = Q3_MSECS;

	setitimer(WHICH, &timer, NULL);

	/* Drop sigmask */
	sigemptyset(&toBlock);
	sigprocmask(SIG_BLOCK, &toBlock, NULL);

	/* Swap contexts */
	swapcontext(&(*curr_temp).data.ctxt, &(*currCtxt).data.ctxt);
}


/* MAIN FUNCTIONS */

/* give CPU pocession to other user level threads voluntarily */
int my_pthread_yield() {
	/* Disable Timer */
	disableTimer();

	// Set status to yield - BUT ONLY IF IT WAS RUNNING, not waiting
	if((*currCtxt).data.stat == P_RUN)
		(*currCtxt).data.stat = P_YIELD;

	// Call Scheduler
	raise(T_SIG);
	return 0;
};

/* create a new thread */
int my_pthread_create(my_pthread_t * thread, pthread_attr_t * attr, void *(*function)(void*), void * arg) {
	disableTimer();

	if(currCtxt == NULL) { // First time any my_pthread function has been evoked (so store main)
		struct sigaction sigAct;
		sigAct.sa_handler = scheduler;
		sigAct.sa_flags = 0;
		sigaction(T_SIG, &sigAct, NULL);

		currCtxt = malloc(sizeof(tcbNode));
		if(currCtxt == NULL){
			//ERROR
			return -1;
		}
		(*currCtxt).next = NULL;

		// Set defaults
		(*currCtxt).data.tID = __sync_fetch_and_add(&idCount, 1); //Fetch and increment idCounter atomically
		(*currCtxt).data.stat = P_RUN;
		(*currCtxt).data.qNum = 1;
		(*currCtxt).data.ret = NULL;
		(*currCtxt).data.w_mutex = NULL;
		(*currCtxt).data.w_tID = 0;

		// Create ucontext
		getcontext(&(*currCtxt).data.ctxt);
	}

	// Reguardless, create new thread and add to running list
	tcbNode * newNode = malloc(sizeof(tcbNode));
	if(newNode == NULL){
		//ERROR
		return -1;
	}
	(*newNode).next = NULL;

	// Set defaults
	(*newNode).data.tID = __sync_fetch_and_add(&idCount, 1); //Fetch and increment idCounter atomically
	(*newNode).data.stat = P_RUN;
	(*newNode).data.qNum = 1;
	(*newNode).data.ret = NULL;
	(*newNode).data.w_mutex = NULL;
	(*newNode).data.w_tID = 0;

	*thread = (*newNode).data.tID; // Give tID back to user

	//Create ucontext
	getcontext(&(*newNode).data.ctxt);
	(*newNode).data.ctxt.uc_link = 0;
 	(*newNode).data.ctxt.uc_stack.ss_sp = malloc(STKSZE);
 	(*newNode).data.ctxt.uc_stack.ss_size = STKSZE;
 	(*newNode).data.ctxt.uc_stack.ss_flags = 0;

	if((*newNode).data.ctxt.uc_stack.ss_sp == NULL){
		//ERROR
		return -1;
	}

	makecontext(&(*newNode).data.ctxt, (void *) function, 1, arg);

	// Place in queue
	enqueue(&q1, newNode);

	my_pthread_yield();
	return 0;
};

/* terminate a thread */
void my_pthread_exit(void * value_ptr) {
	disableTimer();
	(*currCtxt).data.stat = P_EXIT; // Set to exit status
	(*currCtxt).data.ret = value_ptr; // Point to the return value
	my_pthread_yield(); // Release control
};

/* wait for thread termination */
int my_pthread_join(my_pthread_t thread, void **value_ptr) {
	//Here, you'll free the ss_sp within uc_stack
	disableTimer();

	tcbNode * joinable = getExitThread(thread); // Attempt to find it

	if(joinable == NULL) {
		(*currCtxt).data.w_tID = thread; // give it the tid of the thread its waiting on
		(*currCtxt).data.stat = P_WAIT_T; // Set to wait_t status

		my_pthread_yield(); // Release control

		joinable = getExitThread(thread); // For when it's rescheduled
	}

	(*value_ptr) = (*joinable).data.ret; // Get return value
	free(joinable); // Free the allocated space

	//dequipMask(); // Take down sigprocmask
	return 0;
};

//------------------------------------------------------------------------------------------------

/* initial the mutex lock */
int my_pthread_mutex_init(my_pthread_mutex_t *mutex, const pthread_mutexattr_t *mutexattr) {
	struct itimerval res = disableTimer(); // PAUSE TIMER

	if(mutex == NULL){
		// ERROR
		setitimer(WHICH, &res, NULL); // RESUME TIMER
		return -1;
	}

	(*mutex).lock = 0;
	(*mutex).tID = 0;

	setitimer(WHICH, &res, NULL); // RESUME TIMER
	return 0;
};

/* aquire the mutex lock */
int my_pthread_mutex_lock(my_pthread_mutex_t *mutex) {
	struct itimerval res = disableTimer(); // PAUSE TIMER

	if(mutex == NULL){
		// ERROR
		setitimer(WHICH, &res, NULL); // RESUME TIMER
		return -1;
	}
	if((*mutex).lock == -1){
		// ERROR - can't lock destroyed mutex
		setitimer(WHICH, &res, NULL); // RESUME TIMER
		return -1;
	}

	if((*mutex).lock == 0){ // It can be locked
		if(currCtxt == NULL)
			(*mutex).tID = idCount; // For 1-thread only situations
		else
			(*mutex).tID = (*currCtxt).data.tID;
		(*mutex).lock = 1;

		setitimer(WHICH, &res, NULL); // RESUME TIMER

	} else { // Must yield and try again
		if(currCtxt == NULL) {
			// ERROR - can't double lock mutex (still 1 thread only - no timer/yield)
			return -1;
		}

		(*currCtxt).data.stat = P_WAIT_M;
		(*currCtxt).data.w_mutex = mutex;

		my_pthread_yield();

		if((*mutex).lock == -1){
			// ERROR - can't lock destroyed mutex
			return -1; // No timer change
		}

		(*mutex).tID = (*currCtxt).data.tID;
		(*mutex).lock = 1;
	}

	return 0;
};

/* release the mutex lock */
int my_pthread_mutex_unlock(my_pthread_mutex_t *mutex) {
	struct itimerval res = disableTimer(); // PAUSE TIMER

	if(mutex == NULL){
		// ERROR
		setitimer(WHICH, &res, NULL); // RESUME TIMER
		return -1;
	}
	if((*mutex).lock == -1){
		// ERROR - can't unlock destroyed mutex
		setitimer(WHICH, &res, NULL); // RESUME TIMER
		return -1;
	}
	if((*mutex).lock == 0){
		// ERROR - can't unlock if unlocked
		setitimer(WHICH, &res, NULL); // RESUME TIMER
		return -1;
	}

	if(currCtxt == NULL) { // For 1-thread only situations
		(*mutex).lock = 0;
	} else if((*mutex).tID == (*currCtxt).data.tID){ // Make sure tID matches
		(*mutex).lock = 0;
	} else {
		// ERROR - can't unlock if not same thread
		setitimer(WHICH, &res, NULL); // RESUME TIMER
		return -1;
	}

	if(currCtxt != NULL) // Otherwise, only 1 thread so dont yield
		my_pthread_yield();

	return 0;
};

/* destroy the mutex */
int my_pthread_mutex_destroy(my_pthread_mutex_t *mutex) {
	struct itimerval res = disableTimer(); // PAUSE TIMER

	if(mutex == NULL){
		// ERROR
		setitimer(WHICH, &res, NULL); // RESUME TIMER
		return -1;
	}
	if((*mutex).lock == -1){
		// ERROR - already destroyed
		setitimer(WHICH, &res, NULL); // RESUME TIMER
		return -1;
	}
	if((*mutex).lock == 1){
		// ERROR - can't destroy a locked mutex
		setitimer(WHICH, &res, NULL); // RESUME TIMER
		return -1;
	}

	(*mutex).lock = -1;

	setitimer(WHICH, &res, NULL); // RESUME TIMER
	return 0;
};
