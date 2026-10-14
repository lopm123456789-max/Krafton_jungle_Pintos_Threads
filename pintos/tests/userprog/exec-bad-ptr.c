/* exec 시스템 콜에 잘못된 포인터를 넘김.
   프로세스가 종료 코드 -1로 끝나야 함. */

#include <syscall.h>
#include "tests/main.h"

void
test_main (void) 
{
  exec ((char *) 0x20101234);
}
