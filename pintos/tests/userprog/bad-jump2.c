/* 커널 가상 주소의 코드를 실행하려 함.
   프로세스가 종료 코드 -1로 끝나야 함. */

#include "tests/lib.h"
#include "tests/main.h"

void
test_main (void) 
{
  msg ("Congratulations - you have successfully called kernel code: %d", 
        ((int (*)(void))0x8004000000)());
  fail ("should have exited with -1");
}
