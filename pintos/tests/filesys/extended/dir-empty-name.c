/* 이름이 빈 문자열인 디렉터리를 만들어 봄. 실패해야 함. */

#include <syscall.h>
#include "tests/lib.h"
#include "tests/main.h"

void
test_main (void) 
{
  CHECK (!mkdir (""), "mkdir \"\" (must return false)");
}
