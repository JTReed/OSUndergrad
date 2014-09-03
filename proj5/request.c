/*
 * request.c: Does the bulk of the work for the web server.
 */

#include "cs537.h"
#include "request.h"

long int getTime() {
	struct timeval t;
	long time;

	gettimeofday(&t, NULL);
	/* round up to nearest int */ 
	time = (long)((1 + (t.tv_sec * (long)1000000) + t.tv_usec - 1));
	return time;
}

double toMS(long n) 
{
	return (double)(n / 1000.0);
}

void requestError(	int fd,
			char *cause,
			char *errnum,
			char *shortmsg,
			char *longmsg )
{
	char buf[MAXLINE], body[MAXBUF];

	printf("Request ERROR\n");

	/* Create the body of the error message */
	sprintf(body, "<html><title>CS537 Error</title>");
	sprintf(body, "%s<body bgcolor=""fffff"">\r\n", body);
	sprintf(body, "%s%s: %s\r\n", body, errnum, shortmsg);
	sprintf(body, "%s<p>%s: %s\r\n", body, longmsg, cause);
	sprintf(body, "%s<hr>CS537 Web Server\r\n", body);

	/* Write out the header information for this response */
	sprintf(buf, "HTTP/1.0 %s %s\r\n", errnum, shortmsg);
	Rio_writen(fd, buf, strlen(buf));
	printf("%s", buf);

	sprintf(buf, "Content-Type: text/html\r\n");
	Rio_writen(fd, buf, strlen(buf));
	printf("%s", buf);

	sprintf(buf, "Content-Length: %lu\r\n\r\n", strlen(body));
	Rio_writen(fd, buf, strlen(buf));

	/* Write out the content */
	Rio_writen(fd, body, strlen(body));
	printf("%s", body);
}

/*
 * Reads and discards everything up to an empty text line
 */
void requestReadhdrs(rio_t *rp)
{
	char buf[MAXLINE];

	Rio_readlineb(rp, buf, MAXLINE);

	while (strcmp(buf, "\r\n"))
	{
		Rio_readlineb(rp, buf, MAXLINE);
	}

	return;
}

/*
 * Return 1 ifstatic, 0 if dynamic content
 * Calculates filename (and cgiargs, for dynamic) from uri
 */
int requestParseURI(char *uri, char *filename, char *cgiargs)
{
	char *ptr;

	if(!strstr(uri, "cgi"))
	{
		/* static */
		strcpy(cgiargs, "");
		sprintf(filename, ".%s", uri);

		if(uri[strlen(uri)-1] == '/')
		{
			strcat(filename, "home.html");
		}

		return 1;
	}
	else
	{
		/* dynamic */
		ptr = index(uri, '?');

		if(ptr)
		{
			strcpy(cgiargs, ptr+1);

			*ptr = '\0';
		}
		else
		{
			strcpy(cgiargs, "");
		}

		sprintf(filename, ".%s", uri);

		return 0;
	}
}

/*
 * Fills in the filetype given the filename
 */
void requestGetFiletype(char *filename, char *filetype)
{
	if(strstr(filename, ".html"))
	{
		strcpy(filetype, "text/html");
	}
	else if(strstr(filename, ".gif"))
	{
		strcpy(filetype, "image/gif");
	}
	else if(strstr(filename, ".jpg"))
	{
		strcpy(filetype, "image/jpeg");
	}
	else
	{
		strcpy(filetype, "test/plain");
	}
}

void requestServeDynamic(int fd, char *filename, char *cgiargs, request* req)
{
	char buf[MAXLINE], *emptylist[] = {NULL};

	/* The server does only a little bit of the header. */
	/* The CGI script has to finish writing out the header. */
	sprintf(buf, "HTTP/1.0 200 OK\r\n");
	sprintf(buf, "%s Server: Tiny Web Server\r\n", buf);

	/* CS537: Your statistics go here -- fill in the 0's with something
	 * useful!
	 */
	sprintf(buf, "%s Stat-req-arrival: %f\r\n", buf, toMS(req->req_arrival));
	sprintf(buf, "%s Stat-req-dispatch: %f\r\n", buf, toMS(req->req_dispatch));
	sprintf(buf, "%s Stat-thread-id: %d\r\n", buf, req->t_stats.thread_id);
	sprintf(buf, "%s Stat-thread-count: %d\r\n", buf, req->t_stats.thread_count);
	sprintf(buf, "%s Stat-thread-static: %d\r\n", buf, req->t_stats.thread_static);
	sprintf(buf, "%s Stat-thread-dynamic: %d\r\n", buf, req->t_stats.thread_dynamic);

	Rio_writen(fd, buf, strlen(buf));

	if(Fork() == 0)
	{
		/* Child process */
		Setenv("QUERY_STRING", cgiargs, 1);

		/* When the CGI process writes to stdout, it will instead go to
		 * the socket
		 */
		Dup2(fd, STDOUT_FILENO);
		Execve(filename, emptylist, environ);
	}

	Wait(NULL);
}

void requestServeStatic(int fd, char *filename, int filesize, request* req)
{
	int srcfd;
	char *srcp;
	char filetype[MAXLINE];
	char buf[MAXBUF];
	char tmp = 0;
	int i;
	long t, read_start, read_end;

	requestGetFiletype(filename, filetype);

	srcfd = Open(filename, O_RDONLY, 0);

	/* Rather than call read() to read the file into memory, which would
	 * require that we allocate a buffer, we memory-map the file
	 */
	srcp = Mmap(0, filesize, PROT_READ, MAP_PRIVATE, srcfd, 0);

	Close(srcfd);

	req->req_age = *(req->req_disp_count)-req->old_disp_count;

	(*(req->req_disp_count))++;

	/* The following code is only needed to help you time the "read" given
	 * that the file is memory-mapped.
	 * This code ensures that the memory-mapped file is brought into memory
	 * from disk.
	 */

	read_start = getTime();

	/* When you time this, you will see that the first time a client
	 * requests a file, the read is much slower than subsequent requests.
	 */
	for (i = 0; i < filesize; i++)
	{
		tmp += *(srcp+i);
	}

	read_end = getTime();

	req->req_read = read_end - read_start;

	/* the request is complete and we store that time */
	t = getTime();
	req->req_complete = t - req->req_arrival; 

	sprintf(buf, "HTTP/1.0 200 OK\r\n");
	sprintf(buf, "%s Server: CS537 Web Server\r\n", buf);

	/* CS537: Your statistics go here -- fill in the 0's with something
	 * useful!
	 */
	sprintf(buf, "%s Stat-req-arrival: %f\r\n", buf, toMS(req->req_arrival));
	sprintf(buf, "%s Stat-req-dispatch: %f\r\n", buf, toMS(req->req_dispatch));
	sprintf(buf, "%s Stat-req-read: %f\r\n", buf, toMS(req->req_read));
	sprintf(buf, "%s Stat-req-complete: %f\r\n", buf, toMS(req->req_complete));
	sprintf(buf, "%s Stat-req-age: %d\r\n", buf, req->req_age);
	sprintf(buf, "%s Stat-thread-id: %d\r\n", buf, req->t_stats.thread_id);
	sprintf(buf, "%s Stat-thread-count: %d\r\n", buf, req->t_stats.thread_count);
	sprintf(buf, "%s Stat-thread-static: %d\r\n", buf, req->t_stats.thread_static);
	sprintf(buf, "%s Stat-thread-dynamic: %d\r\n", buf, req->t_stats.thread_dynamic);

	sprintf(buf, "%s Content-Length: %d\r\n", buf, filesize);
	sprintf(buf, "%s Content-Type: %s\r\n\r\n", buf, filetype);

	Rio_writen(fd, buf, strlen(buf));

	/* Writes out to the client socket the memory-mapped file */
	Rio_writen(fd, srcp, filesize);
	Munmap(srcp, filesize);
}

void preRequestHandle(request* req)
{
	int fd = req->fd;
	int is_static;
	struct stat sbuf;
	char buf[MAXLINE], method[MAXLINE], uri[MAXLINE], version[MAXLINE];
	char filename[MAXLINE], cgiargs[MAXLINE];

	rio_t rio;

	Rio_readinitb(&rio, fd);
	Rio_readlineb(&rio, buf, MAXLINE);
	sscanf(buf, "%s %s %s", method, uri, version);

	printf("%s %s %s\n", method, uri, version);

	if(strcasecmp(method, "GET"))
	{
		requestError(	fd,
				method,
				"501",
				"Not Implemented",
				"CS537 Server does not implement this method" );

		return;
	}

	requestReadhdrs(&rio);

	is_static = requestParseURI(uri, filename, cgiargs);

	if(stat(filename, &sbuf) < 0)
	{
		requestError(	fd,
				filename,
				"404",
				"Not found",
				"CS537 Server could not find this file" );

		return;
	}

	req->ready = 1;
	req->sbuf = sbuf;
	req->is_static = is_static;
	req->cgiargs = cgiargs;

	/* TODO: Free req->filename in worker thread */
	req->filename = malloc(sizeof(char)*MAXLINE);
	strncpy(req->filename, filename, MAXLINE);
}

/* handle a request */
void requestHandle(request* req)
{
	/* set the request's dispatch time */
	long t;
	t = getTime();
	req->req_dispatch = t - req->req_arrival;

	if(req->is_static)
	{
		if(!(S_ISREG(req->sbuf.st_mode))
		|| !(S_IRUSR & req->sbuf.st_mode))
		{
			requestError(	req->fd,
					req->filename,
					"403",
					"Forbidden",
					"CS537 Server could not read this file" );

			return;
		}

		requestServeStatic(	req->fd,
					req->filename,
					req->sbuf.st_size,
					req );
	}
	else
	{
		if(!(S_ISREG(req->sbuf.st_mode))
		|| !(S_IXUSR & req->sbuf.st_mode))
		{
			requestError(	req->fd,
					req->filename,
					"403",
					"Forbidden",
					"CS537 Server could not run this CGI program" );

			return;
		}

		requestServeDynamic(	req->fd,
					req->filename,
					req->cgiargs,
					req );
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
