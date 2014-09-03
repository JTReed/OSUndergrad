#include "counter.h"

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <pthread.h>
#include <sys/time.h>

#define MAX_THREADS 20
#define INCREMENT_COUNT 50000
#define MAX_REPS 3

typedef struct counter_arg
{
	counter_t* counter;
}
counter_arg;

double get_time()
{
	struct timeval tv;

	gettimeofday(&tv, NULL);

	return tv.tv_sec+(tv.tv_usec/1000000.0);
}

void *increment_count(void* arg)
{
	counter_t* counter = ((counter_arg *)arg)->counter;
	int count = INCREMENT_COUNT;

	while(--count >= 0)
	{
		Counter_Increment(counter);
	}

	return NULL;
}

float parallel_increment(int thread_count)
{
	double begin_time;
	double end_time;
	counter_t counter;
	counter_arg *arg = malloc(sizeof(counter_arg)*thread_count);
	pthread_t *thread = malloc(sizeof(pthread_t)*thread_count);
	int index;

	Counter_Init(&counter, 0);

	begin_time = get_time();

	for(index = 0; index < thread_count; index++)
	{
		arg[index].counter = &counter;

		pthread_create(	&thread[index],
				NULL,
				increment_count,
				&arg[index] );
	}

	for(index = 0; index < thread_count; index++)
	{
		pthread_join(thread[index], NULL);
	}

	end_time = get_time();

	free(arg);
	free(thread);

	return (float)(end_time-begin_time);
}

int main()
{
	int thread_count = 1;
	int rep_count = 0;
	float time_arr[MAX_THREADS] = {0};

	while (rep_count < MAX_REPS)
	{
		while (thread_count <= MAX_THREADS)
		{

			time_arr[thread_count-1]
				+= parallel_increment(thread_count);

			thread_count++;
		}

		thread_count = 1;
		rep_count++;
	}

	while (thread_count <= MAX_THREADS)
	{
		printf(	"%d %f\n",
			thread_count,
			time_arr[thread_count-1]/(float)MAX_REPS );

		thread_count++;
	}

	return 0;
}

/*
 * Local variables:
 * c-basic-offset: 8
 * tab-width: 8
 * indent-tabs-mode: t
 * End:
 *
 * vi: set shiftwidth=8 tabstop=8 noexpandtab:
 * :indentSize=8:tabSize=8:noTabs=false:
 */
