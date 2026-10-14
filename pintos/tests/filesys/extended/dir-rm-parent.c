/* 현재 디렉터리의 부모를 지워 봄. 그 디렉터리가 비어 있지
   않으므로 실패해야 함. */

#include <syscall.h>
#include "tests/lib.h"
#include "tests/main.h"

void
test_main (void) 
{
  CHECK (mkdir ("a"), "mkdir \"a\"");
  CHECK (chdir ("a"), "chdir \"a\"");
  CHECK (mkdir ("b"), "mkdir \"b\"");
  CHECK (chdir ("b"), "chdir \"b\"");
  CHECK (!remove ("/a"), "remove \"/a\" (must fail)");
}
