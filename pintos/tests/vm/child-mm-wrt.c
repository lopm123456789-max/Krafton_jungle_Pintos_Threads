/* mmap-exit의 자식 프로세스.
   파일을 mmap 하고 매핑을 통해 쓴 다음, munmap을 부르지 않고
   종료함. 매핑된 영역의 데이터는 프로그램이 끝날 때 파일에
   기록돼야 함. */

#include <string.h>
#include <syscall.h>
#include "tests/vm/sample.inc"
#include "tests/lib.h"
#include "tests/main.h"

#define ACTUAL ((void *) 0x10000000)

void
test_main (void)
{
  int handle;

  CHECK (create ("sample.txt", sizeof sample), "create \"sample.txt\"");
  CHECK ((handle = open ("sample.txt")) > 1, "open \"sample.txt\"");
  CHECK (mmap (ACTUAL, sizeof sample, 1, handle, 0) != MAP_FAILED, "mmap \"sample.txt\"");
  memcpy (ACTUAL, sample, sizeof sample);
}

