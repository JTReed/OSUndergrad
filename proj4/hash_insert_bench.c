#include "hash.h"

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <pthread.h>
#include <sys/time.h>

#define MAX_THREADS 20
#define INSERT_COUNT 50000
#define BUCKET_COUNT 20
#define DATA_PTR (void *)0xdeadbeef
#define MAX_REPS 3

typedef struct insert_arg
{
	hash_t* hash;
	int offset;
}
insert_arg;

double get_time()
{
	struct timeval tv;

	gettimeofday(&tv, NULL);

	return tv.tv_sec+(tv.tv_usec/1000000.0);
}

void *insert_elements(void* arg)
{
	hash_t* hash = ((insert_arg *)arg)->hash;
	int offset = ((insert_arg *)arg)->offset;
	int count = INSERT_COUNT;

	while(--count >= 0)
	{
		Hash_Insert(hash, DATA_PTR, offset+count);
	}

	return NULL;
}

float parallel_insert(int thread_count)
{
	double begin_time;
	double end_time;
	hash_t hash;
	insert_arg *arg = malloc(sizeof(insert_arg)*thread_count);
	pthread_t *thread = malloc(sizeof(pthread_t)*thread_count);
	int index = 0;

	Hash_Init(&hash, BUCKET_COUNT);

	begin_time = get_time();

	for(index = 0; index < thread_count; index++)
	{
		arg[index].hash = &hash;
		arg[index].offset = index*INSERT_COUNT;

		pthread_create(	&thread[index],
				NULL,
				insert_elements,
				&arg[index] );
	}

	for(index = 0; index < thread_count; index++)
	{
		pthread_join(thread[index], NULL);
	}

	end_time = get_time();

	Hash_Destroy(&hash);

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
		while(thread_count <= MAX_THREADS)
		{
			time_arr[thread_count-1]
				+= parallel_insert(thread_count);

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
