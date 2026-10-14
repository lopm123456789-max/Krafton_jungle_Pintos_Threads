/* 루트 디렉터리를 지워 봄.
   실패해야 함. */

#include <syscall.h>
#include "tests/lib.h"
#include "tests/main.h"

void
test_main (void) 
{
  CHECK (!remove ("/"), "remove \"/\" (must fail)");
  CHECK (create ("/a", 243), "create \"/a\"");
}
