/* check if Km_alloc have padding around allocate chunks
 * 
 *
 */

#include <stdio.h>
#include <unistd.h>
#include "mem.h"
#include "common.h"

int main()
{
  int err, i, j;
  void *p[5];
  int sizes[5] = {88, 488, 48, 488, 188};
  
  err = Mem_Init(4096, 1);
  if(err == -1)
    return -1;
  
  for(i=0; i<5; i++) {
    p[i] = Mem_Alloc(sizes[i]);
    if(p[i] == NULL)
      return -1;
    
    if (isStalePadding(p[i], sizes[i]))
      return -1;
  }
  return 0;
}

