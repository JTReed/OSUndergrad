#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/select.h>
#include "udp.h"
#include "mfs.h"

enum command
{
	CMD_LOOKUP,
	CMD_STAT,
	CMD_WRITE,
	CMD_READ,
	CMD_CREAT,
	CMD_UNLINK
};

#define MAX_PORT_TRIES (10)
char buffer[BUFFER_SIZE];
struct sockaddr_in addr;
int sd;

int send_command(enum command cmd, ...)
{
	va_list arg;
	int rc;
	int res_ready = 0;
	struct sockaddr_in addr2;
	char cmd_buffer[BUFFER_SIZE];
	void *output;

	/* variables that might need to be resent */
	int pinum, inum, block, type;
	char* name;
	char* buffer;

	if(cmd == CMD_LOOKUP)
	{
		va_start(arg, cmd);

		pinum = va_arg(arg, int);
		name = va_arg(arg, char *);
		output = va_arg(arg, int *);

		va_end(arg);

		sprintf(cmd_buffer, "LOOKUP %d %s", pinum, name);
	}
	else if(cmd == CMD_STAT)
	{
		va_start(arg, cmd);

		inum = va_arg(arg, int);
		output = va_arg(arg, MFS_Stat_t *);

		va_end(arg);

		sprintf(cmd_buffer, "STAT %d", inum);
	}
	else if(cmd == CMD_WRITE)
	{
		char new_buffer[MFS_BLOCK_SIZE+1];

		va_start(arg, cmd);

		inum = va_arg(arg, int);
		buffer = va_arg(arg, char *);
		block = va_arg(arg, int);

		va_end(arg);

		/* Make sure the buffer will fit */
		memcpy(new_buffer, buffer, MFS_BLOCK_SIZE);

		new_buffer[MFS_BLOCK_SIZE] = '\0';

		sprintf(cmd_buffer, "WRITE %d %s %d", inum, new_buffer, block);
	}
	else if(cmd == CMD_READ)
	{
		va_start(arg, cmd);

		inum = va_arg(arg, int);
		output = va_arg(arg, char *);
		block = va_arg(arg, int);

		va_end(arg);

		sprintf(cmd_buffer, "READ %d %d", inum, block);
	}
	else if(cmd == CMD_CREAT)
	{
		va_start(arg, cmd);

		pinum = va_arg(arg, int);
		type = va_arg(arg, int);
		name = va_arg(arg, char *);

		va_end(arg);

		sprintf(cmd_buffer, "CREAT %d %d %s", pinum, type, name);
	}
	else if(cmd == CMD_UNLINK)
	{
		va_start(arg, cmd);

		pinum = va_arg(arg, int);
		name = va_arg(arg, char *);

		va_end(arg);

		sprintf(cmd_buffer, "UNLINK %d %s", pinum, name);
	}

	/* write buffer to server@specified-port */
	rc = UDP_Write(sd, &addr, cmd_buffer, BUFFER_SIZE);

	printf("CLIENT:: sent buffer (%d)\n", rc);

	do
	{
		/* check timeout */
		struct timeval timeout;
		fd_set fds;

		/* set time limit */
		timeout.tv_sec = 5;
		timeout.tv_usec = 0;

		/* create a descriptor set containing socket */
		FD_ZERO(&fds);
		FD_SET(sd, &fds);

		rc = select(sd+1, &fds, NULL, NULL, &timeout);

		/* no data for specified no. of seconds */
		if(rc == 0)
		{
			/* re-send command */
			rc = UDP_Write(sd, &addr, cmd_buffer, BUFFER_SIZE);

			printf("CLIENT:: resent buffer (%d)\n", rc);
		}
		else if(rc > 0)
		{
			/* read buffer from ... */
			rc = UDP_Read(sd, &addr2, cmd_buffer, BUFFER_SIZE);

			printf(	"CLIENT:: read %d bytes (buffer: '%s')\n",
				rc,
				cmd_buffer );

			res_ready = 1;
		}
	}
	while(!res_ready);

	/* Command successful */
	if(rc > 0
	&& strncmp(cmd_buffer+strlen(cmd_buffer)-2, "OK", BUFFER_SIZE) == 0)
	{
		if(cmd == CMD_LOOKUP)
		{
			memcpy(	output,
				cmd_buffer+strlen(cmd_buffer)+1,
				sizeof(int) );
		}
		else if(cmd == CMD_STAT)
		{
			memcpy(	output,
				cmd_buffer+strlen(cmd_buffer)+1,
				sizeof(MFS_Stat_t) );
		}
		else if(cmd == CMD_READ)
		{
			memcpy(	output,
				cmd_buffer+strlen(cmd_buffer)+1,
				MFS_BLOCK_SIZE );
		}
	}
	else
	{
		rc = 0;
	}

	return (rc > 0)-1;
}

int MFS_Init(char *hostname, int port)
{
	int rc = -1;

	sd = UDP_Open(0);

	if(sd > -1)
	{
		/* contact server at specified port */
		rc = UDP_FillSockAddr(&addr, hostname, port);
	}

	return rc;
}

int MFS_Lookup(int pinum, char *name)
{
	int inum;
	int result = send_command(CMD_LOOKUP, pinum, name, &inum);

	printf(	"LOOKUP REPLY: %d %d\n", result, inum);

	return (result == 0)?inum:-1;
}

int MFS_Stat(int inum, MFS_Stat_t *m)
{
	return send_command(CMD_STAT, inum, m);
}

int MFS_Write(int inum, char *buffer, int block)
{
	return send_command(CMD_WRITE, inum, buffer, block);
}

int MFS_Read(int inum, char *buffer, int block)
{
	return send_command(CMD_READ, inum, buffer, block);
}

int MFS_Creat(int pinum, int type, char *name)
{
	return send_command(CMD_CREAT, pinum, type, name);
}

int MFS_Unlink(int pinum, char *name)
{
	return send_command(CMD_UNLINK, pinum, name);
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
