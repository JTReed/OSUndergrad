#include "mem.h"

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <stdbool.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>

//#define _debug

#pragma pack(1)
typedef struct ChunkHeader {
	unsigned int size;
 	struct ChunkHeader* next; // Points to the next /free/ block
} ChunkHeader;
#pragma pack()

ChunkHeader* m_poolHead;
void* m_startOfPool;
int m_debug;
int m_error;
//lvoid* brokenValue;
int m_initialized;
void* m_padding = (void*)0xABCDDCBA;

int Mem_Init(int sizeOfRegion, int debug)
{
	if (sizeOfRegion < 1 || m_initialized) {
		//printf("Mem_Init: Bad arguments\n");
		m_error = E_BAD_ARGS;
		return -1;
	}

	// open the /dev/zero device
	int fd = open("/dev/zero", O_RDWR);
	
	// sizeOfRegion (in bytes) needs to be easily divisible by the page size
	int pageSize = getpagesize();
	if (sizeOfRegion % pageSize != 0) {
		// round region size up to the nearest page size 
		sizeOfRegion += pageSize - (sizeOfRegion % pageSize);
		//printf("Pool size rounded up to %d\n", sizeOfRegion);
	}

	//printf("Initializing pool of %d bytes\n\n", sizeOfRegion);
	
	void* memPool = mmap(NULL, sizeOfRegion + sizeof(ChunkHeader), PROT_READ | PROT_WRITE, MAP_PRIVATE, fd, 0);
	m_startOfPool = memPool;

	if(memPool == MAP_FAILED) {
		//printf("Mem_Init: mmap failed\n");
		perror("mmap");
		return -1;
	}

	// create header so we can access the pool throughout the program
	m_poolHead = memPool;
	m_poolHead->size = sizeOfRegion;
	m_poolHead->next = NULL;
	//printf("Struct is %d bytes\n", sizeof(ChunkHeader));
	
	// close the device
	close(fd);
	m_initialized = 1;
	//printf("start of pool = 0x%X\n", m_startOfPool);

	if(debug) {
		// fill free memory with 0xDEADBEEF 
		m_debug = 1;
		void* filler = (void*)0xDEADBEEF;
		int i;
		for(i = sizeof(ChunkHeader); i < (sizeOfRegion + sizeof(ChunkHeader)); i += 4) {
			memcpy(memPool + i, &filler, sizeof(filler));
		}
	}

	/*int i;
	for(i = 0; i < sizeOfRegion; i++) {
		printf("%c", (char*)(memPool + i));
	}*/

	//brokenValue = &((ChunkHeader*)(memPool + (sizeof(ChunkHeader) * 2) + 512 + 128))->size;
	//printf("mempool = %X, brokenValue = %X\n", memPool, brokenValue);


	return 0;
}

void* Mem_Alloc(int size)
{
	//printf("Allocating %d bytes\n", size);
	
	ChunkHeader* currentNode = m_poolHead;
	ChunkHeader* previousNode = NULL;
	int count = 1;
	
	if (!m_initialized) {
		// Mem_Init has not been called, list is empty
		//printf("Mem_Alloc: not initialized\n");
		m_error = E_NO_SPACE;
		return NULL;
	} 
	
	if (size < 1) {
		//printf("mem_Alloc: size 0\n");
		m_error = E_BAD_ARGS;
		return NULL;
	}
	
	// 4 byte aligned for performance - round up
	if (size % 4 != 0) {
		size += 4 - (size % 4);
		//printf("Size rounded up to %d to 4-byte align\n", size);
	}	
	
	//printf("current node size = %d\n", currentNode->size);
	// do until last node (points to NULL)
	do {
		if (currentNode->size < size + sizeof(ChunkHeader) + (128 * m_debug)) {
			//printf("current size = %d, size needed = %d\n", currentNode->size, size + sizeof(ChunkHeader));
			//printf("current node 0x%X, next node 0x%X\n", currentNode, currentNode->next);
			count++;
			if(currentNode->next != (ChunkHeader*)-1) { 
				//printf("setting previous node\n");
				previousNode = currentNode; 
			}
			currentNode = currentNode->next;
		} else {
			break;
		}
	} while (currentNode != NULL);
	//printf("inserting at node %d\n", count);
	//printf("\nallocating size %d in free node of size %d\n\n", size, currentNode->size);
	if (currentNode == NULL) {
		// No nodes big enough
		//printf("Not enough space\n");
		m_error = E_NO_SPACE;
		return NULL;
	}

	
	//printf("arrived at node size = %d\n", currentNode->size);
	void* returnAddress = (void*)currentNode + sizeof(ChunkHeader) + (64 * m_debug);
	
	ChunkHeader* nodeToAdd = returnAddress + size + (64 * m_debug);
	nodeToAdd->size = currentNode->size - (size + sizeof(ChunkHeader));
	nodeToAdd->next = currentNode->next;
	
	currentNode->size = size;
	currentNode->next = (ChunkHeader*)-1;
	
	if(previousNode != NULL) {
		// connect a previous free node if it exists
		previousNode->next = nodeToAdd;
	}
	
	if (currentNode == m_poolHead) {
	// replace the head of the list if needed
		m_poolHead = nodeToAdd;
	}
	
	//printf("Allocated %d bytes of space at 0x%X\n", currentNode->size, returnAddress); 
	//printf("created new node header with size %d\n", nodeToAdd->size);
	
#ifdef _debug
	// DEBUG PRINT IT OUT
	printf("After alloc: ");
	ChunkHeader* curr = (ChunkHeader*)m_startOfPool;
	do {
		if(curr->next == (ChunkHeader*)-1) { printf("_"); } // print '_' if taken
		if(curr == m_poolHead) { printf("*"); } // print * if head
		printf("%d, ", curr->size);
		
		// go to next chunk if taken
		if(curr->next != NULL) { curr = (ChunkHeader*)((void*)curr + sizeof(ChunkHeader) + curr->size + (128*m_debug)); } 
		else { curr = curr->next; }
	} while(curr != NULL);
	printf("NULL\n");
	
	ChunkHeader* cur = m_poolHead;
	printf("Free List: ");
	do {
		printf("%d, ", cur->size);
		cur = cur->next;
	} while(cur != NULL);
	printf("NULL\n");
	// DEBUG PRINT
#endif

	if(m_debug) {
		// debug stuff
		int i;
		for(i = 0; i < 64; i += 4) {
			memcpy((void*)currentNode + sizeof(ChunkHeader) + i, &m_padding, 4);
		}

		for(i = 0; i < 64; i += 4) {
			memcpy((void*)currentNode + sizeof(ChunkHeader) + 64 + currentNode->size + i - 1, &m_padding, 4);
		}

	}

	
	return returnAddress;
}

int Mem_Free(void* ptr)
{
	int success = -1;
	int count = 1;
	
	ChunkHeader* nodeToFree = (ChunkHeader*)(ptr - sizeof(ChunkHeader) - (64 * m_debug));
	//printf("size of header to remove = %d\n", nodeToFree->size);
	
	// check if... ptr is null, nodetofree is not taken
	if (ptr == NULL) {
		// nothing to free, no harm done
		return 0;
	}
	if (nodeToFree->next != (ChunkHeader*)-1) {
		//printf("not previously malloc'd\n");
		m_error = E_BAD_POINTER;
		return -1;
	}
	
	// iterate through all nodes to find out where we are
	ChunkHeader* currentNode = (ChunkHeader*)m_startOfPool;
	//printf("on node of size %d\n", currentNode->size);
	ChunkHeader* lastFreeNode = NULL;
	do {
		if(currentNode == nodeToFree) {
			// we've found the correct node!
			if (currentNode < m_poolHead) {
				// new head - so let's replace the old one and point to it
				nodeToFree->next = m_poolHead;
				//printf("nodeToFree is of size %d\n", nodeToFree->size);
				//printf("nodetofree-> next is of size %d\n", nodeToFree->next->size);
				m_poolHead = nodeToFree;
				//printf("freed successfully at head\n");
				success = 0;
				break;
			} else {
				// after the head, so lets point the last free node to it
				nodeToFree->next = lastFreeNode->next;
				//printf("nodeToFree->next is of size %d\n", nodeToFree->next->size);
				lastFreeNode->next = nodeToFree;
				//printf("this node is of size %d and lastnode's next is of size %d\n", nodeToFree->size, lastFreeNode->next->size);
				//printf("freed successfully inside list\n");
				success = 0;
				break;
			}
		}
		
		if (currentNode->next != NULL) {
			// this is not the last node, so we have some stuff to do
			if(currentNode->next != (ChunkHeader*)-1) {
				// the current node is free, so let's make sure we remember it
				lastFreeNode = currentNode;
			}

			// increment to the next node in line
			currentNode = (ChunkHeader*)((void*)currentNode + sizeof(ChunkHeader) + currentNode->size + (128 * m_debug));
			//printf("on node of size %d\n", currentNode->size);
			count++;
		} else {
			currentNode = NULL;
		}
	} while (currentNode != NULL);

	// coalesce with surrounding free nodes
	// merge with free node to right
	if(nodeToFree->next != NULL && (ChunkHeader*)((void*)currentNode + sizeof(ChunkHeader) + currentNode->size + (128*m_debug)) == currentNode->next) {
		//printf("merging to right\n");
		nodeToFree->size += sizeof(ChunkHeader) + nodeToFree->next->size + (128*m_debug);
		nodeToFree->next = nodeToFree->next->next;
	}

	// merge with free node on the left if necessary
	if(lastFreeNode != NULL) {
		// Only merge to the left if this is adjacent to a free node
		if((ChunkHeader*)((void*)lastFreeNode + sizeof(ChunkHeader) + lastFreeNode->size + (128*m_debug)) == nodeToFree) {
			//printf("merging to left\n");
			lastFreeNode->size += sizeof(ChunkHeader) + nodeToFree->size + (128*m_debug);
			lastFreeNode->next = nodeToFree->next;
			nodeToFree = lastFreeNode;
		}
	}
	//printf("freed at node %d\n", count);
	
#ifdef _debug
	// DEBUG PRINT
	printf("After free: ");
	ChunkHeader* curr = (ChunkHeader*)m_startOfPool;
	do {
		// TODO breaking here
		if(curr->next == (ChunkHeader*)-1) { printf("_"); } // print '_' if taken
		if(curr == m_poolHead) { printf("*"); } // print * if head
		printf("%d, ", curr->size);
		
		// go to next chunk if taken
		if(curr->next != NULL) { 
			curr = (ChunkHeader*)((void*)curr + sizeof(ChunkHeader) + curr->size + (128 * m_debug)); 
		} 
		else { 
			curr = curr->next; 
		}
	} while(curr != NULL);
	printf("NULL\n"); 
	
	ChunkHeader* cur = m_poolHead;
	printf("Free List: ");
	do {
		printf("%d, ", cur->size);
		cur = cur->next;
	} while(cur != NULL);
	printf("NULL\n");
	// DEBUG PRINT
#endif

	if(m_debug) {
		//write all free space with 0xDEADBEEF
		void* filler = (void*)0xDEADBEEF;
		int i;
		for(i = sizeof(ChunkHeader); i < nodeToFree->size; i += sizeof(filler)) {
			memcpy((void*)nodeToFree + i, &filler, sizeof(filler));
		}
	}

	return success;
	
}

void Mem_Dump()
{
	
}
