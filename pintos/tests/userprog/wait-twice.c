/* 자식 프로세스가 끝날 때까지 두 번 기다림.
   첫 번째 호출은 평소처럼 기다렸다가 종료 코드를 돌려줘야 함.
   두 번째 wait 호출은 즉시 -1을 돌려줘야 함. */

#include <syscall.h>
#include "tests/lib.h"
#include "tests/main.h"

void
test_main (void) 
{
  pid_t child;
  if ((child = fork ("child-simple"))){
    msg ("wait(exec()) = %d", wait (child));
    msg ("wait(exec()) = %d", wait (child));
  } else {
    exec ("child-simple");
  }
}
