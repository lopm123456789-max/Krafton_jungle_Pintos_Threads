/* 자식 프로세스에 인자가 제대로 전달되는지 테스트함. */

#include <syscall.h>
#include "tests/lib.h"
#include "tests/main.h"

void
test_main (void) 
{
  msg ("I'm your father");
  exec ("child-args childarg");
}
