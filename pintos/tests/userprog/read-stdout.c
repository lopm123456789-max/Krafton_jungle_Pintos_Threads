/* fd 1(stdout)에서 읽어 봄.
   그냥 실패하거나 프로세스가 종료 코드 -1로 끝날 수 있음. */

#include <stdio.h>
#include <syscall.h>
#include "tests/main.h"

void
test_main (void) 
{
  char buf;
  read (STDOUT_FILENO, &buf, 1);
}
