/* Checks for behaviour using successive Mem_Alloc()s and Mem_Free()s  
   for BEST FIT
*/

#include <stdio.h>
#include <unistd.h>
#include "mem.h"

int main()
{
    int err, i, j;
    void *p[6];
    int pagesize = 0;
    int nr_iterations = 5;
    int size;
    printf("starting\n");

    pagesize = getpagesize();    

    err = Mem_Init(4 * pagesize, 0);
    if(err == -1)
    	printf("init fail\n");
        return -1;

    for(j=0; j< nr_iterations; j++) {
            size = pagesize/32;
	    for(i=0; i<6; i++) {
	         p[i] =  Mem_Alloc(size);
	         printf("allocated %d bytes at 0x%X\n", size, p[i]); 
	         if(p[i] == NULL) {
	         	   printf("alloc fail with j = %d and i = %d\n", j, i);
	               return -1;
	         }
	         err = Mem_Free(p[i]);
	         if(err != 0) {
	         	   printf("free fail with j = %d and i = %d\n", j, i);
	               return -1;
	         }
             size = size * 2;
	    }
    }    

    return 0;
}

