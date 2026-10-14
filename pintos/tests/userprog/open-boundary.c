/* 이름이 두 페이지의 경계에 걸친 파일을 만듦.
   올바른 동작이므로 성공해야 함. */

#include <syscall.h>
#include "tests/userprog/boundary.h"
#include "tests/lib.h"
#include "tests/main.h"

void
test_main (void) 
{
  CHECK (open (copy_string_across_boundary ("sample.txt")) > 1,
         "open \"sample.txt\"");
}
