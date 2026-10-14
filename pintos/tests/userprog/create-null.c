/* 널 포인터를 이름으로 파일을 만들어 봄.
   프로세스가 종료 코드 -1로 끝나야 함. */

#include "tests/lib.h"
#include "tests/main.h"

void
test_main (void) 
{
  msg ("create(NULL): %d", create (NULL, 0));
}
