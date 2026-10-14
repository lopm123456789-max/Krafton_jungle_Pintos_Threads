/* wait-killed 테스트가 실행하는 자식 프로세스.
   pintos를 실행하려 하는데, Pintos 안에는 `pintos`가 없으므로
   프로세스가 종료 코드 -1로 끝나야 함. */

#include "tests/lib.h"
#include "tests/main.h"

void
test_main (void) 
{
  exec ("pintos");
  fail ("should have exited with -1");
}
