/* 디렉터리를 만들었다가 지우고, 정말 없어졌는지 확인함. */

#include <syscall.h>
#include "tests/lib.h"
#include "tests/main.h"

void
test_main (void) 
{
  CHECK (mkdir ("a"), "mkdir \"a\"");
  CHECK (remove ("a"), "rmdir \"a\"");
  CHECK (!chdir ("a"), "chdir \"a\" (must return false)");
}
