/* 이름이 두 페이지의 경계에 걸친 파일을 엶.
   올바른 동작이므로 성공해야 함. */

#include <syscall.h>
#include "tests/userprog/boundary.h"
#include "tests/lib.h"
#include "tests/main.h"

void
test_main (void) 
{
  msg ("create(\"quux.dat\"): %d",
       create (copy_string_across_boundary ("quux.dat"), 0));
}
