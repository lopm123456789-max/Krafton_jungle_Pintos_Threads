/* 길이 0인 파일을 매핑해 봄. 성공하든 실패하든 상관없지만
   프로세스가 종료되거나 죽으면 안 됨.
   그다음 매핑하려던 주소를 역참조하면 프로세스가 종료 코드 -1로
   끝나야 함. */

#include <syscall.h>
#include "tests/lib.h"
#include "tests/main.h"

void
test_main (void)
{
  char *data = (char *) 0x7f000000;
  int handle;

  CHECK (create ("empty", 0), "create empty file \"empty\"");
  CHECK ((handle = open ("empty")) > 1, "open \"empty\"");

  /* mmap() 호출은 성공할 수도 실패할 수도 있음. 상관없음. */
  msg ("mmap \"empty\"");
  mmap (data, 0, 0, handle, 0);

  /* 호출이 성공했든 아니든 *data는 프로세스를 종료시켜야 함. */
  fail ("unmapped memory is readable (%d)", *data);
}

