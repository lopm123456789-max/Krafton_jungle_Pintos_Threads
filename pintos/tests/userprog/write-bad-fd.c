/* 잘못된 fd에 쓰려 함.
   조용히 실패하거나 프로세스가 종료 코드 -1로 끝나야 함. */

#include <limits.h>
#include <syscall.h>
#include "tests/main.h"

void
test_main (void) 
{
  char buf = 123;
  write (0x01012342, &buf, 1);
  write (7, &buf, 1);
  write (2546, &buf, 1);
  write (-5, &buf, 1);
  write (-8192, &buf, 1);
  write (INT_MIN + 1, &buf, 1);
  write (INT_MAX - 1, &buf, 1);
}
