/* exit 시스템 콜 테스트. */

#include "tests/lib.h"
#include "tests/main.h"

void
test_main (void) 
{
  exit (57);
  fail ("should have called exit(57)");
}
