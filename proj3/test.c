#include "mem.h"

#include <stdio.h>

int main(int argc, void* argv[])
{
	Mem_Init(4096, 1);
	
	// ALLOCATE JUNK
	char* ptr = Mem_Alloc(512);
	//printf("Allocated memory at 0x%X ptr\n", ptr);
	char* ptr2 = Mem_Alloc(128);
	//printf("Allocated memory at 0x%X ptr2\n", ptr2);
	char* ptr3 = Mem_Alloc(8);
	//printf("Allocated memory at 0x%X ptr3\n", ptr3);
	//Mem_Free(ptr3);
	//printf("freed ptr3\n");
	char* ptr4 = Mem_Alloc(16);
	//printf("Allocated memory at 0x%X ptr4\n", ptr4);
	char* ptr5 = Mem_Alloc(32);
	//printf("Allocated memory at 0x%X ptr5\n", ptr5);
	//Mem_Free(ptr);
	//printf("freed ptr\n");
	char* ptr6 = Mem_Alloc(64);
	//printf("Allocated memory at 0x%X ptr6\n", ptr6);

	//printf("\nNOW ATTEMPTING TO FREE\n\n");
	
	// FREE STUFF
	Mem_Free(ptr);
	Mem_Free(ptr2);
	//printf("freed ptr\n");
	//Mem_Free(ptr2);
	//printf("freed ptr2\n");
	Mem_Free(ptr3);
	//printf("freeing ptr2\n");
	Mem_Free(ptr4);
	//printf("freed ptr4\n");
	//printf("freeing ptr5\n");
	Mem_Free(ptr5);
	//printf("freed ptr5\n");
	//printf("freeing ptr5\n");
	Mem_Free(ptr6);
	//printf("freed ptr6\n");
	
	//ptr = Mem_Alloc(256);
}
