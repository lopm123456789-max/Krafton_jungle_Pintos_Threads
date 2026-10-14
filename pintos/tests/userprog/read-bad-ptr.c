/* read 시스템 콜에 잘못된 포인터를 넘김.
   프로세스가 종료 코드 -1로 끝나야 함. */

#include <syscall.h>
#include "tests/lib.h"
#include "tests/main.h"

void
test_main (void) 
{
  int handle;
  CHECK ((handle = open ("sample.txt")) > 1, "open \"sample.txt\"");

  read (handle, (char *) 0xc0100000, 123);
  fail ("should not have survived read()");
}
