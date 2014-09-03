#ifndef __REQUEST_H__

typedef struct thread_stats
{
	/* Thread stats */
	int thread_id;
	int thread_count;
	int thread_static;
	int thread_dynamic;
}
thread_stats;

typedef struct request
{
	int fd;
	int ready;
	int is_static;
	int epoch;
	struct stat sbuf;
	char* filename;
	char* cgiargs;
	/* Request stats */
	long int req_arrival;
	long int req_dispatch;
	long int req_read;
	long int req_complete;
	int req_age;
	thread_stats t_stats;
	/* Not displayed */
	int* req_disp_count;
	int old_disp_count;
}
request;

long int getTime();
void preRequestHandle(request* req);
void requestHandle(request* req);

#endif

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
