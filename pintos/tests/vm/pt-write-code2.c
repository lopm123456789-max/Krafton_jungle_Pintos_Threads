/* 시스템 콜을 이용해 코드 세그먼트에 쓰려 함.
   프로세스가 종료 코드 -1로 끝나야 함. */

#include "tests/lib.h"
#include "tests/main.h"

void
test_main (void)
{
  int handle;

  CHECK ((handle = open ("sample.txt")) > 1, "open \"sample.txt\"");
  read (handle, (void *) test_main, 1);
  fail ("survived reading data into code segment");
}
