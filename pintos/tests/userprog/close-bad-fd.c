/* 잘못된 fd를 닫으려 함. 조용히 실패하거나 종료 코드 -1로
   끝나야 함. */

#include <syscall.h>
#include "tests/main.h"

void
test_main (void) 
{
  close (0x20101234);
}
