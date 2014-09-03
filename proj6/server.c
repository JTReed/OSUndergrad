#include <stdio.h>
#include <unistd.h>
#include <math.h>
#include <sys/stat.h>
#include "udp.h"
#include "mfs.h"

/* The size is in bytes */
#define BLOCK_SIZE (4096)
#define BITMAP_SIZE (BITMAP_COUNT/8)
#define FS_SIZE (DATA_REGION_OFFSET+DATA_COUNT*BLOCK_SIZE)

#define BITMAP_COUNT (4096)
#define INODE_COUNT (4096)
#define DATA_COUNT (4096)
#define DIR_ENT_COUNT (BLOCK_SIZE/sizeof(MFS_DirEnt_t))

#define INODE_BITMAP_OFFSET (0)
#define DATA_BITMAP_OFFSET (INODE_BITMAP_OFFSET+BITMAP_SIZE)
#define INODE_REGION_OFFSET (DATA_BITMAP_OFFSET+BITMAP_SIZE)
#define DATA_REGION_OFFSET (INODE_REGION_OFFSET+INODE_COUNT*sizeof(inode))

typedef struct inode
{
	int type;
	int size;
	int blocks;
	int pointer[10];
}
inode;

int bitmap_alloc(int fd, int begin_offset)
{
	int offset = -1;
	int bit_offset = -1;
	char *bitmap = malloc(BITMAP_SIZE);

	lseek(fd, begin_offset, SEEK_SET);
	read(fd, bitmap, BITMAP_SIZE);

	/* Find non-full byte */
	while(*(bitmap+(++offset)) == 0xffffffff && offset < BITMAP_SIZE);

	/* Proceed only if a usable byte is found */
	if(offset < BITMAP_SIZE)
	{
		/* Find free bit */
		while((*(bitmap+offset)&(0x1<<(++bit_offset))) != 0);

		/* Set bit */
		*(bitmap+offset) |= (0x1<<(bit_offset));

		lseek(fd, begin_offset, SEEK_SET);
		write(fd, bitmap, BITMAP_SIZE);
	}

	free(bitmap);

	return (offset < BITMAP_SIZE)?(offset*8)+bit_offset:-1;
}

int bitmap_check(int fd, int begin_offset, int index)
{
	int result = -1;
	char *bitmap = malloc(BITMAP_SIZE);

	lseek(fd, begin_offset, SEEK_SET);
	read(fd, bitmap, BITMAP_SIZE);

	if(index >= 0 && index < BITMAP_SIZE)
	{
		result = ((*(bitmap+(index/8))&(0x1<<(index%8))) > 0);
	}

	free(bitmap);

	return result;
}

int init_obj(int fd, int inum, int pinum, int type, char *name)
{
	int result = 1;
	int found = 0;
	int dnum;
	int pdir_block_index = -1;
	int pdir_ent_index = -1;
	MFS_DirEnt_t pdir_ent[DIR_ENT_COUNT];
	inode parent_inode;

	result = (	result
			&& inum >= 0
			&& inum < BITMAP_SIZE
			&& pinum >=0
			&& pinum < BITMAP_SIZE );

	/* Find free entry in parent */
	if(result && inum != pinum)
	{
		/* Get the parent inode */
		lseek(fd, INODE_REGION_OFFSET+pinum*sizeof(inode), SEEK_SET);
		read(fd, &parent_inode, sizeof(inode));

		/* Iterate through the blocks until an empty entry is found */
		while(!found && ++pdir_block_index < parent_inode.blocks)
		{
			/* Fill pdir_ent with the blocks the parent points to */
			lseek(	fd,
				DATA_REGION_OFFSET
				+parent_inode.pointer[pdir_block_index]
				*BLOCK_SIZE,
				SEEK_SET );

			read(fd, pdir_ent, sizeof(pdir_ent));

			pdir_ent_index = -1;

			while(!found && ++pdir_ent_index < DIR_ENT_COUNT)
			{
				found = (pdir_ent[pdir_ent_index].inum == -1);
			}
		}

		result = (result && (found || parent_inode.blocks < 10));
	}

	/* Update parent directory */
	if(result && inum != pinum)
	{
		/* Allocate new block */
		if(!found && parent_inode.blocks < 10)
		{
			int dnum = bitmap_alloc(fd, DATA_BITMAP_OFFSET); 

			result = (result && dnum >= 0 && dnum < BITMAP_COUNT);

			if(result)
			{
				pdir_block_index = parent_inode.blocks;

				/* Set block pointer and update block count */
				parent_inode.pointer[parent_inode.blocks++]
					= dnum;

				for(	pdir_ent_index = 0;
					pdir_ent_index < DIR_ENT_COUNT;
					pdir_ent_index++ )
				{
					pdir_ent[pdir_ent_index].inum = -1;
				}

				pdir_ent_index = 0;
			}
		}

		/* Set the new directory's inum and name */
		pdir_ent[pdir_ent_index].inum = inum;

		strncpy(pdir_ent[pdir_ent_index].name, name, 252);

		parent_inode.size += sizeof(MFS_DirEnt_t);

		/* Overwrite the old parent entry with the new one */
		lseek(	fd,
			DATA_REGION_OFFSET
			+parent_inode.pointer[pdir_block_index]
			*BLOCK_SIZE,
			SEEK_SET );

		write(fd, pdir_ent, BLOCK_SIZE);

		lseek(fd, INODE_REGION_OFFSET+pinum*sizeof(inode), SEEK_SET);
		write(fd, &parent_inode, sizeof(inode));
	}

	if(result && type == MFS_DIRECTORY)
	{
		int dir_ent_index = 1; /* Skip first two entries */
		MFS_DirEnt_t dir_ent[DIR_ENT_COUNT];

		/* Set inum and name up for new dir and set all block pointers
		 * to empty
		 */
		dir_ent[0].inum = inum;
		strncpy(dir_ent[0].name, ".", 2);

		dir_ent[1].inum = pinum;
		strncpy(dir_ent[1].name, "..", 3);

		while(++dir_ent_index < DIR_ENT_COUNT)
		{
			dir_ent[dir_ent_index].inum = -1;
		}

		dnum = bitmap_alloc(fd, DATA_BITMAP_OFFSET);

		lseek(fd, DATA_REGION_OFFSET+dnum*BLOCK_SIZE, SEEK_SET);
		write(fd, &dir_ent, BLOCK_SIZE);

		printf(	"created directory %s with inum %d and pinum %d\n",
			name,
			inum,
			pinum );
	}

	return result?dnum:-1;
}

int init_fs(char *filename)
{
	/* The specs doesn't say anything about the superblock, so we don't
	 * create one.
	 */
	int fd = open(filename, O_RDWR|O_CREAT|O_SYNC, 0666);
	int result = (fd >= 0);
	struct stat fs_stat;

	result = (result && fstat(fd, &fs_stat) == 0);

	if(fs_stat.st_size != FS_SIZE)
	{
		inode self;

		/* ftruncate() fills extra space with zeros */
		result = (result && ftruncate(fd, FS_SIZE) == 0);

		/* bitmap_alloc() should return 0 since there should be no
		 * allocated inode at this point.
		 */
		result = (result && bitmap_alloc(fd, INODE_BITMAP_OFFSET) == 0);

		result = (	result
				&& init_obj(fd, 0, 0, MFS_DIRECTORY, NULL)
				> -1 );

		/* Write inode */
		self.type = MFS_DIRECTORY;
		self.size = 2*sizeof(MFS_DirEnt_t);
		self.blocks = 1;
		self.pointer[0] = 0;

		lseek(fd, INODE_REGION_OFFSET, SEEK_SET);
		write(fd, &self, sizeof(inode));
	}

	return result?fd:-1;
}

int handle_lookup(	int fd,
			int pinum,
			char *name,
			int *output_size,
			void **output )
{
	inode parent;
	MFS_DirEnt_t pdir_ent[DIR_ENT_COUNT];
	int block_index = -1;
	int ent_index = -1;
	int found = 0;
	int result =1;

	printf("HANDLE LOOKUP\n");

	result = (result && pinum >= 0 && pinum < BITMAP_COUNT);

	/* Read parent inode from filesystem */
	lseek(fd, INODE_REGION_OFFSET+pinum*sizeof(inode), SEEK_SET);
	read(fd, &parent, sizeof(inode));

	printf("PARENT: %d %d %d\n", parent.type, parent.size, parent.blocks);

	/* Iterate through the blocks until an empty entry is found */
	while(!found && ++block_index < parent.blocks)
	{
		/* Fill pdir_ent with the blocks the parent points to */
		lseek(	fd,
			DATA_REGION_OFFSET
			+parent.pointer[block_index]
			*BLOCK_SIZE,
			SEEK_SET );

		read(fd, pdir_ent, sizeof(pdir_ent));

		ent_index = -1;

		while(!found && ++ent_index < DIR_ENT_COUNT)
		{
			MFS_DirEnt_t entry = pdir_ent[ent_index];

			found = (	entry.inum != -1
					&& strncmp(entry.name, name, 252) == 0);
		}
	}

	result = (result && found);

	if(result)
	{
		*output = malloc(sizeof(int));
		*(int *)(*output) = pdir_ent[ent_index].inum;

		*output_size = sizeof(int);

		printf(	"File with name %s and parent %d has inum %d\n",
			name,
			pinum,
			pdir_ent[ent_index].inum );
	}

	return result-1;
}

int handle_stat(int fd, int inum, int *output_size, void **output)
{
	inode node;
	MFS_Stat_t *stat = NULL;
	int result = 1;

	printf("HANDLE STAT\n");

	result = (result && inum >= 0 && inum < BITMAP_COUNT);

	/* Check if inode is allocated */
	if(result)
	{
		char *inode_bitmap = malloc(BITMAP_SIZE);

		lseek(fd, INODE_BITMAP_OFFSET, SEEK_SET);
		read(fd, inode_bitmap, BITMAP_SIZE);

		result = (	result
				&& (inode_bitmap[inum/8]&(0x1<<(inum%8)))
				!= 0 );
	}

	if(result)
	{
		stat = malloc(sizeof(MFS_Stat_t));

		/* Load up inode */
		lseek(fd, INODE_REGION_OFFSET+inum*sizeof(inode), SEEK_SET);
		read(fd, &node, sizeof(inode));

		stat->type = node.type;
		stat->size = node.size;
		stat->blocks = node.blocks;

		printf("type: %d, size: %d, blocks: %d\n",
			stat->type,
			stat->size,
			stat->blocks);

		*output = stat;
		*output_size = sizeof(MFS_Stat_t);
	}

	return result-1;
}

int handle_write(int fd, int inum, char *buffer, int block)
{
	int result = 1;
	inode self;

	printf("HANDLE WRITE\n");

	result = (	result
			&& inum >= 0
			&& inum < BITMAP_COUNT
			&& block >= 0
			&& block < 10 );

	if(result)
	{
		lseek(fd, INODE_REGION_OFFSET+inum*sizeof(inode), SEEK_SET);
		read(fd, &self, sizeof(inode));

		result = (	result
				&& self.type == MFS_REGULAR_FILE
				&& self.blocks >= block );

		if(block == self.blocks)
		{
			self.pointer[block]
				= bitmap_alloc(fd, DATA_BITMAP_OFFSET);

			self.blocks++;
		}

		self.size += BLOCK_SIZE;

		lseek(	fd,
			DATA_REGION_OFFSET+self.pointer[block]*BLOCK_SIZE,
			SEEK_SET );

		write(fd, buffer, BLOCK_SIZE);

		lseek(fd, INODE_REGION_OFFSET+inum*sizeof(inode), SEEK_SET);
		write(fd, &self, sizeof(inode));
	}

	return result-1;
}

int handle_read(int fd, int inum, int block, int *output_size, void **output)
{
	inode read_node;
	int result = 1;

	printf("HANDLE READ\n");

	result = (	result
			&& inum >= 0
			&& inum < BITMAP_COUNT
			&& block >= 0
			&& block < 10
			&& bitmap_check(fd, INODE_BITMAP_OFFSET, inum) );

	if(result)
	{
		/* Read in inode */
		lseek(fd, INODE_REGION_OFFSET+inum*sizeof(inode), SEEK_SET);
		read(fd, &read_node, sizeof(inode));

		result = (result && block < read_node.blocks);
	}

	if(result)
	{
		if(read_node.type == MFS_REGULAR_FILE)
		{
			*output = malloc(BLOCK_SIZE);

			lseek(	fd,
				DATA_REGION_OFFSET
				+read_node.pointer[block]
				*BLOCK_SIZE,
				SEEK_SET );

			read(fd, *output, BLOCK_SIZE);

			*output_size = BLOCK_SIZE;
		}
		else if(read_node.type == MFS_DIRECTORY)
		{
			*output = malloc(BLOCK_SIZE);

			/* Load up relevent block from inode pointers */
			lseek(	fd,
				DATA_REGION_OFFSET
				+read_node.pointer[block]
				*BLOCK_SIZE,
				SEEK_SET);

			read(fd, *output, BLOCK_SIZE);

			*output_size = BLOCK_SIZE;
		}
		else
		{
			result = 0;
		}
	}

	return result-1;
}

int handle_creat(int fd, int pinum, int type, char *name)
{
	int result = 1;
	int parent_index = -1;
	int inum;
	MFS_DirEnt_t parent[DIR_ENT_COUNT];
	inode new_inode;

	printf("HANDLE CREAT\n");

	result = (	result
			&& pinum >= 0
			&& pinum < BITMAP_COUNT
			&& bitmap_check(fd, INODE_BITMAP_OFFSET, pinum) );

	/* Check if file exists; return success if it is */
	if(result)
	{
		lseek(fd, DATA_REGION_OFFSET+pinum*BLOCK_SIZE, SEEK_SET);
		read(fd, parent, sizeof(parent));

		while(	++parent_index < DIR_ENT_COUNT
			&& (parent[parent_index].inum == -1
			|| strncmp(parent[parent_index].name, name, 252) != 0));
	}

	/* File doesn't exist */
	if(result && parent_index == DIR_ENT_COUNT)
	{
		inum = bitmap_alloc(fd, INODE_BITMAP_OFFSET);

		new_inode.type = type;
		new_inode.size = (type == MFS_DIRECTORY)*2*sizeof(MFS_DirEnt_t);
		new_inode.blocks = (type == MFS_DIRECTORY); /* 1 if directory */
		new_inode.pointer[0] = init_obj(fd, inum, pinum, type, name);

		result = (result && new_inode.pointer[0] > -1);

		lseek(fd, INODE_REGION_OFFSET+inum*sizeof(inode), SEEK_SET);
		write(fd, &new_inode, sizeof(inode));
	}

	return result-1;
}

int handle_unlink(int fd, int pinum, char *name)
{
	inode self;
	inode parent;
	MFS_DirEnt_t pdir_ent[DIR_ENT_COUNT];
	int inum = -1;
	int result = 1;
	int pent_index = -1;

	printf("HANDLE UNLINK\n");

	lseek(fd, INODE_REGION_OFFSET+pinum*sizeof(inode), SEEK_SET);
	read(fd, &parent, sizeof(inode));

	lseek(fd, DATA_REGION_OFFSET+parent.pointer[0]*BLOCK_SIZE, SEEK_SET);
	read(fd, pdir_ent, sizeof(pdir_ent));

	result = (	result
			&& pinum >= 0
			&& pinum < 10
			&& bitmap_check(fd, INODE_BITMAP_OFFSET, pinum)
			&& parent.type == MFS_DIRECTORY);

	if(result)
	{
		/* Find file's entry in parent directory */
		while(	++pent_index < DIR_ENT_COUNT
			&& (pdir_ent[pent_index].inum == -1
			|| strncmp(pdir_ent[pent_index].name, name, 252)
			!= 0) );
	}

	/* File found */
	if(result && pent_index != DIR_ENT_COUNT)
	{
		inum = pdir_ent[pent_index].inum;

		lseek(fd, INODE_REGION_OFFSET+inum*sizeof(inode), SEEK_SET);
		read(fd, &self, sizeof(inode));

		result = (	result
				&& (self.type == MFS_REGULAR_FILE
				|| (self.type == MFS_DIRECTORY
				&& self.size == 2*sizeof(MFS_DirEnt_t))) );
	}

	if(result && pent_index != DIR_ENT_COUNT)
	{
		char *inode_bitmap = malloc(BITMAP_SIZE);
		char *data_bitmap = malloc(BITMAP_SIZE);

		lseek(fd, INODE_BITMAP_OFFSET, SEEK_SET);
		read(fd, inode_bitmap, BITMAP_SIZE);

		lseek(fd, DATA_BITMAP_OFFSET, SEEK_SET);
		read(fd, data_bitmap, BITMAP_SIZE);

		/* Free entry in parent */
		pdir_ent[pent_index].inum = -1;

		parent.size -= sizeof(MFS_DirEnt_t);

		/* Clear inode bit */
		inode_bitmap[inum/8] &= ~(0x1<<(inum%8));

		/* Clear data bits */
		while(self.blocks-- > 0)
		{
			int dnum = self.pointer[self.blocks];

			data_bitmap[dnum/8] &= ~(0x1<<(dnum%8));
		}

		lseek(fd, INODE_REGION_OFFSET+inum*sizeof(inode), SEEK_SET);
		write(fd, &self, sizeof(inode));
		
		lseek(fd, INODE_REGION_OFFSET+pinum*sizeof(inode), SEEK_SET);
		write(fd, &parent, sizeof(inode));

		lseek(fd, INODE_REGION_OFFSET+pinum*sizeof(inode), SEEK_SET);
		write(fd, &parent, sizeof(inode));

		lseek(fd, INODE_BITMAP_OFFSET, SEEK_SET);
		write(fd, inode_bitmap, BITMAP_SIZE);

		lseek(fd, DATA_BITMAP_OFFSET, SEEK_SET);
		write(fd, data_bitmap, BITMAP_SIZE);

		free(inode_bitmap);
	}

	/* No need to write inode back to disk; clearing the bit in the inode
	 * bitmap should be sufficient.
	 */

	lseek(fd, DATA_REGION_OFFSET+parent.pointer[0]*BLOCK_SIZE, SEEK_SET);
	write(fd, pdir_ent, sizeof(pdir_ent));

	return result-1;
}

int handle_command(int fd, char *command, char *response)
{
	int i = 0;
	int result = -1;
	int output_size = 0;
	void *output = NULL;
	char *token[3]; /* maximum possible tokens for a valid command is 3 */

	/* Inject null terminator to make sure strtok() will not cause buffer
	 * overflow
	 */
	command[BUFFER_SIZE-1] = '\0';

	strncpy(response, strtok(command, " "), BUFFER_SIZE);

	do
	{
		token[i] = strtok(NULL, " ");
	}
	while(i < 2 && token[i++] != NULL);

	if(strncmp(command, "LOOKUP", strlen("LOOKUP")) == 0)
	{
		result = handle_lookup(	fd,
					atoi(token[0]),
					token[1],
					&output_size,
					&output );
	}
	else if(strncmp(command, "STAT", strlen("STAT")) == 0)
	{
		result = handle_stat(fd, atoi(token[0]), &output_size, &output);
	}
	else if(strncmp(command, "WRITE", strlen("WRITE")) == 0)
	{
		result = handle_write(	fd,
					atoi(token[0]),
					token[1],
					atoi(token[2]) );
	}
	else if(strncmp(command, "READ", strlen("READ")) == 0)
	{
		result = handle_read(	fd,
					atoi(token[0]),
					atoi(token[1]),
					&output_size,
					&output );
	}
	else if(strncmp(command, "CREAT", strlen("CREAT")) == 0)
	{
		result = handle_creat(	fd,
					atoi(token[0]),
					atoi(token[1]),
					token[2] );
	}
	else if(strncmp(command, "UNLINK", strlen("UNLINK")) == 0)
	{
		result = handle_unlink(fd, atoi(token[0]), token[1]);
	}

	if(result == 0)
	{
		strncat(response, " OK", BUFFER_SIZE-strlen(response));
	}
	else
	{
		strncat(response, " FAIL", BUFFER_SIZE-strlen(response));
	}

	if(result == 0 && output)
	{
		/* The output, if any, will be just beyond the null terminator.
		 * This makes it easy to print out the reponse without having
		 * binary data interferes with the output.
		 */
		memcpy(response+strlen(response)+1, output, output_size);
	}

	return result;
}

int main(int argc, char *argv[])
{
	int portid;
	int fd;
	int sd;

	if(argc != 3)
	{
		printf("Usage: server [portnum] [file-system-image]\n");

		exit(1);
	}

	portid = atoi(argv[1]);
	sd = UDP_Open(portid); /* port # */

	assert(sd > -1);

	fd = init_fs(argv[2]);

	printf("waiting in loop\n");

	while(1)
	{
		struct sockaddr_in s;
		char buffer[BUFFER_SIZE];

		/* Read message buffer from port sd */
		int rc = UDP_Read(sd, &s, buffer, BUFFER_SIZE);

		if(rc > 0)
		{
			char reply[BUFFER_SIZE];

			/* TODO: handle return values */
			handle_command(fd, buffer, reply);

			printf(	"SERVER:: read %d bytes (message: '%s')\n",
				rc,
				buffer );

			/* Write message buffer to port sd */
			rc = UDP_Write(sd, &s, reply, BUFFER_SIZE);
		}
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
