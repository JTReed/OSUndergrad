/* 
 * Mem_Free with stale padding
 *
 */

#include <stdio.h>
#include <unistd.h>
#include <string.h>
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
  }

  for (i = 0;i < 5;i++)
    // now corrupt the the padding, at one end
    memset(p[i] - 32, 0xAA, 32);

  // call mem free and expect an error return
  int corrupted = 5;
  for (i = 0; i < 5; i++) {
    err = Mem_Free(p[i]);
    if (err == -1 && m_error == E_PADDING_OVERWRITTEN) {
      corrupted--;
    }
    else {
      // goto fail
    }
  }
  
  if (corrupted == 0) return 0;
  else return -1;

  return 0;
}

