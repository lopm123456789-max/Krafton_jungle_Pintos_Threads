/* open 시스템 콜에 잘못된 포인터를 넘김.
   프로세스가 종료 코드 -1로 끝나야 함. */

#include <syscall.h>
#include "tests/lib.h"
#include "tests/main.h"

void
test_main (void) 
{
  msg ("open(0x20101234): %d", open ((char *) 0x20101234));
  fail ("should have called exit(-1)");
}
