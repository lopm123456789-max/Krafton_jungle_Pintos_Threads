/* fd 0(stdin)에 써 봄.
   그냥 실패하거나 프로세스가 종료 코드 -1로 끝날 수 있음. */

#include <syscall.h>
#include "tests/lib.h"
#include "tests/main.h"

void
test_main (void) 
{
  char buf = 123;
  write (0, &buf, 1);
}
