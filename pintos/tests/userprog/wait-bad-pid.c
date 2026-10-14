/* 잘못된 pid를 기다림. 실패하거나 프로세스가 종료 코드 -1로
   끝날 수 있음. */

#include <syscall.h>
#include "tests/main.h"

void
test_main (void) 
{
  wait ((pid_t) 0x0c020301);
}
