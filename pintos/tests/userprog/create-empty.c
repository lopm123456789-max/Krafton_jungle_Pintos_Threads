/* 빈 문자열을 이름으로 파일을 만들어 봄. */

#include "tests/lib.h"
#include "tests/main.h"

void
test_main (void) 
{
  msg ("create(\"\"): %d", create ("", 0));
}
