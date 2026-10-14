/* 스택 포인터보다 4,096바이트 아래 주소에서 읽음.
   프로세스가 종료 코드 -1로 끝나야 함. */

#include <string.h>
#include "tests/arc4.h"
#include "tests/cksum.h"
#include "tests/lib.h"
#include "tests/main.h"

void
test_main (void)
{
  asm volatile ("movq -4096(%rsp), %rax");
}
