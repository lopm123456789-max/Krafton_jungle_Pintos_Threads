/* 커널 메모리를 읽으려 함.
   프로세스가 종료 코드 -1로 끝나야 함. */

#include "tests/lib.h"
#include "tests/main.h"

void
test_main (void) 
{
  msg ("Congratulations - you have successfully read kernel memory: %d", 
        *(int *)0x8004000000);
  fail ("should have exited with -1");
}
