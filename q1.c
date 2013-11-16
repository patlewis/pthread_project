#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include "timer.h"
#include <stdint.h>

/* Function prototype */
void* thread_func(void*);

/* Global vairables */
pthread_t *threads;	// the dynamic array containing all the threads
long nthreads;		// the number of threads total

int main(int argc, char* argv[])
{
	//an error message in case the input is bad
	char message[] = {"Usage:\t ./q1 <nthreads>\nwhere nthreads is an integer representing the number of threads to use (nthreads > 0)."};
	double start, end;

	/* Error checking and input sanitizing.  If there's any error messages,
	 * we'll end prematurely.
	 */
	if (argc != 2)
	{
		printf("%s\n", message);
		return 0;
	}
	nthreads = (long) strtol(argv[1], NULL, 10);
	if (nthreads < 1)
	{
		printf("%s\n", message);
		return 0;
	}

	// create room for all the threads, also error checking
	threads = (pthread_t *)malloc(nthreads * sizeof(pthread_t));
	if (threads == NULL)
	{
		printf("There was an error allocating memory for the threads. I don't know what went wrong. Try again.\n");
		return 1;
	}

	// now to actually create all the threads
	pthread_attr_t attr;
	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
	int i;
	
	GET_TIME(start);
	for(i = 0; i < nthreads; i++)
	{
		pthread_create(&threads[i], &attr, thread_func, (void *) (intptr_t) i);
	}	

	//they're done, now time to join them
	for(i = 0; i < nthreads; i++)
	{
		pthread_join(threads[i], NULL);
	}
	GET_TIME(end);

	//output results
	printf("nthreads: %ld\t total time: %.9f\t time per thread: %.9f\n",  \
	nthreads, end-start, (double) (end-start)/nthreads);

	//cleanup and exit
	free(threads);
	return 0;
}

void* thread_func(void* data)
{
	//long rank = (long) (intptr_t) data;
	//printf("Hello from thread %ld of %ld\n", rank, nthreads);
	return NULL;
}