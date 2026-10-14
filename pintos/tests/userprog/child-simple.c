/* exec-multiple, exec-one, wait-simple, wait-twice 테스트가 실행하는
   자식 프로세스.
   메시지 하나만 출력하고 종료함. */

#include <stdio.h>
#include "tests/lib.h"

int
main (void) 
{
  test_name = "child-simple";

  msg ("run");
  return 81;
}
