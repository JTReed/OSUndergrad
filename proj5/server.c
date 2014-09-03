#include "cs537.h"
#include "request.h"
#include <limits.h>

#define USAGE_STR "Usage: %s <port> <threads> <buffers> <schedalg> [N]\n"

#define FIFO (0)
#define SFF (1)
#define SFF_BS (2)

typedef struct request_handle_arg
{
	request *req;
	int *thread_id;
	int *req_count;
	int *req_index;
	int epoch;
	int epoch_req_count;
	int epoch_size;
	int req_buffer_size;
	int schedalg;
	pthread_cond_t *main_cv;
	pthread_cond_t *worker_cv;
	pthread_mutex_t *mutex;
}
request_handle_arg;

/*
 * server.c: A very, very simple web server
 *
 * To run:
 *	server <portnum (above 2000)>
 *
 * Repeatedly handles HTTP requests sent to this port number.
 * Most of the work is done within routines written in request.c
 */

int get_min_request(request* req, int size, int epoch)
{
	int i;
	int min_index = -1;

	if(!req)
	{
		size = 0;
	}

	for(i = 0; i < size; i++)
	{
		if(req[i].fd != -1 && (epoch == -1 || req[i].epoch == epoch))
		{
			if(!req[i].ready)
			{
				preRequestHandle(&req[i]);
			}

			if(min_index == -1)
			{
				min_index = i;
			}
			else
			{
				if(req[i].sbuf.st_size
				< req[min_index].sbuf.st_size)
				{
					min_index = i;
				}
			}
		}
	}

	return min_index;
}

void *pthread_request_handle(void* arg)
{
	request_handle_arg* req_arg = (request_handle_arg*)arg;
	request req;
	thread_stats t_stats;
	char *extension;
	int req_index;

	t_stats.thread_count = 0;
	t_stats.thread_static = 0;
	t_stats.thread_dynamic = 0;

	pthread_mutex_lock(req_arg->mutex);

	t_stats.thread_id = *(req_arg->thread_id);

	(*(req_arg->thread_id))++;

	pthread_mutex_unlock(req_arg->mutex);

	while(1)
	{
		/* wait for cv */
		pthread_mutex_lock(req_arg->mutex);

		while(*(req_arg->req_count) == 0)
		{
			pthread_cond_wait(req_arg->worker_cv, req_arg->mutex);
		}

		req_index = *(req_arg->req_index);

		if(req_arg->schedalg == FIFO)
		{
			/* Set old_index to 0 if req_index is 0 */
			int old_index = (req_index != 0)*(req_index-1);

			for(	;req_index != old_index
				&& (req_arg->req)[req_index].fd == -1;
				req_index = (req_index+1)
				%req_arg->req_buffer_size );

			if(!(req_arg->req)[req_index].ready)
			{
				preRequestHandle(&(req_arg->req)[req_index]);
			}
		}
		else if(req_arg->schedalg == SFF)
		{
			req_index = get_min_request
					(	req_arg->req,
						req_arg->req_buffer_size,
						-1 );
		}
		else if(req_arg->schedalg == SFF_BS)
		{
			req_index = get_min_request
					(	req_arg->req,
						req_arg->req_buffer_size,
						req_arg->epoch );

			req_arg->epoch_req_count
				= (req_arg->epoch_req_count+1)
				% req_arg->epoch_size;

			if(req_arg->epoch_req_count == 0)
			{
				req_arg->epoch = (req_arg->epoch+1)%INT_MAX;
			}
		}

		req = (req_arg->req)[req_index];

		(req_arg->req)[req_index].fd = -1;

		(*(req_arg->req_count))--;

		(*(req_arg->req_index))
			= (req_index+1)%(req_arg->req_buffer_size);

		pthread_mutex_unlock(req_arg->mutex);

		t_stats.thread_count++;

		extension = strstr(req.filename, ".cgi");

		/* extension[4] is guaranteed to be within the allocated buffer
		 * by strstr()
		 */
		if(extension != NULL && extension[4] == '\0')
		{
			t_stats.thread_dynamic++;
		}
		else
		{
			t_stats.thread_static++;
		}

		req.t_stats = t_stats;

		requestHandle(&req);

		Close(req.fd);

		pthread_mutex_lock(req_arg->mutex);

		pthread_cond_signal(req_arg->main_cv);

		pthread_mutex_unlock(req_arg->mutex);
	}
}

void getargs(	int *port,
		int *thread_pool_size,
		int *conn_buffer_size,
		int *schedalg,
		int *epoch_size,
		int argc,
		char *argv[])
{
	int error = 0;

	if(argc >= 5)
	{
		*port = atoi(argv[1]);
		*thread_pool_size = atoi(argv[2]);
		*conn_buffer_size = atoi(argv[3]);

		if(*thread_pool_size < 1 || *conn_buffer_size < 1)
		{
			error = 1;
		}

		if(!error && argc == 5)
		{
			if(strcmp(argv[4], "FIFO") == 0)
			{
				*schedalg = FIFO;
			}
			else if(strcmp(argv[4], "SFF") == 0)
			{
				*schedalg = SFF;
			}
			else
			{
				error = 1;
			}
		}
		else if(!error && argc == 6 && strcmp(argv[4], "SFF-BS") == 0)
		{
			*schedalg = SFF_BS;
			*epoch_size = atoi(argv[5]);

			if(*epoch_size < 1)
			{
				error = 1;
			}
		}
		else
		{
			error = 1;
		}
	}
	else
	{
		error = 1;
	}

	if(error)
	{
		fprintf(stderr, USAGE_STR, argv[0]);

		exit(1);
	}
}

int main(int argc, char *argv[])
{
	int listenfd;
	int thread_pool_size;
	int req_buffer_size;
	int req_count = 0;
	int port;
	int schedalg;
	int epoch_size = 0;
	int epoch = 0;
	int epoch_req_count = 0;
	int req_disp_count = 0;
	int req_serve_index = 0;
	int thread_id = 0;
	int i = 0;
	request* req;
	request_handle_arg thread_arg;
	pthread_mutex_t mutex;
	pthread_cond_t worker_cv;
	pthread_cond_t main_cv;
	pthread_t* thread;

	getargs(	&port,
			&thread_pool_size,
			&req_buffer_size,
			&schedalg,
			&epoch_size,
			argc,
			argv );

	req = malloc(sizeof(request)*req_buffer_size);
	thread = malloc(sizeof(pthread_t)*thread_pool_size);

	for(i = 0; i < req_buffer_size; i++)
	{
		req[i].fd = -1;
	}

	/* CS537: create some worker threads using pthread_create ... */
	pthread_cond_init(&worker_cv, NULL);
	pthread_cond_init(&main_cv, NULL);
	pthread_mutex_init(&mutex, NULL);

	thread_arg.req = req;
	thread_arg.req_count = &req_count;
	thread_arg.req_index = &req_serve_index;
	thread_arg.req_buffer_size = req_buffer_size;
	thread_arg.thread_id = &thread_id;
	thread_arg.epoch = 0;
	thread_arg.epoch_req_count = 0;
	thread_arg.epoch_size = epoch_size;
	thread_arg.schedalg = schedalg;
	thread_arg.mutex = &mutex;
	thread_arg.worker_cv = &worker_cv;
	thread_arg.main_cv = &main_cv;

	for(i = 0; i < thread_pool_size; i++)
	{
		pthread_create(	&thread[i],
				NULL,
				pthread_request_handle,
				&thread_arg );
	}

	i = 0;
	listenfd = Open_listenfd(port);

	while (1)
	{
		request new_req;
		struct sockaddr_in clientaddr;
		int clientlen = sizeof(clientaddr);
		int old_i;
		long t;

		/* Wait until buffer become non-full */
		pthread_mutex_lock(thread_arg.mutex);

		while(req_count == req_buffer_size)
		{
			pthread_cond_wait(&main_cv, &mutex);
		}

		old_i = (i != 0)*(old_i-1);

		for(;i != old_i && req[i].fd != -1; i = (i+1)%req_buffer_size);

		pthread_mutex_unlock(thread_arg.mutex);

		new_req.fd = Accept(	listenfd,
					(SA *)&clientaddr,
					(socklen_t *)&clientlen );

		t = getTime();

		new_req.req_arrival = t;

		new_req.ready = 0;
		pthread_mutex_lock(thread_arg.mutex);

		if(schedalg == SFF_BS)
		{
			new_req.epoch = epoch;

			epoch_req_count = (epoch_req_count+1)%epoch_size;
		}

		if(epoch_req_count == 0)
		{
			epoch = (epoch+1)%INT_MAX;
		}

		new_req.old_disp_count = req_disp_count;
		new_req.req_disp_count = &req_disp_count;

		req[i] = new_req;

		req_count++;

		/* wake thread up */
		pthread_cond_signal(thread_arg.worker_cv);

		pthread_mutex_unlock(thread_arg.mutex);

		i = (i+1)%req_buffer_size;
	}
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
