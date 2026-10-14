/* 잘못된 fd로 mmap 하려 함.
   조용히 실패하거나 프로세스가 종료 코드 -1로 끝나야 함. */

#include <syscall.h>
#include "tests/lib.h"
#include "tests/main.h"

void
test_main (void) 
{
  CHECK (mmap ((void *) 0x10000000, 4096, 0, 0x5678, 0) == MAP_FAILED,
         "try to mmap invalid fd");
}

