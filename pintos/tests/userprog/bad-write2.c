/* 커널 메모리에 쓰려 함.
   프로세스가 종료 코드 -1로 끝나야 함. */

#include "tests/lib.h"
#include "tests/main.h"

void
test_main (void) 
{
  *(int *)0x8004000000 = 42;
  fail ("should have exited with -1");
}
