/* 존재하지 않는 프로세스를 실행하려 함.
   exec 시스템 콜은 -1을 돌려줘야 함. */

#include <syscall.h>
#include "tests/lib.h"
#include "tests/main.h"

void
test_main (void) 
{
  msg ("exec(\"no-such-file\"): %d", exec ("no-such-file"));
}
