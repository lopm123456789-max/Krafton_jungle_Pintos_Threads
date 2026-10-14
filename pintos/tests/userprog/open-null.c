/* 널 포인터를 이름으로 파일을 열어 봄.
   프로세스가 종료 코드 -1로 끝나야 함. */

#include <stddef.h>
#include <syscall.h>
#include "tests/main.h"

void
test_main (void) 
{
  open (NULL);
}
