/* 이미 있는 디렉터리와 같은 이름으로 파일을 만들어 봄.
   실패해야 함. */

#include <syscall.h>
#include "tests/lib.h"
#include "tests/main.h"

void
test_main (void) 
{
  CHECK (mkdir ("abc"), "mkdir \"abc\"");
  CHECK (!create ("abc", 0), "create \"abc\" (must return false)");
}
